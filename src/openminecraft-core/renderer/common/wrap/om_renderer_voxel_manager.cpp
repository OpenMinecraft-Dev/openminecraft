#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_segbuf.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_segbuf.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/world/om_world_chunk.hpp"
#include "openminecraft/world/om_world_chunkmanager.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

namespace openminecraft::renderer::common::wrap
{
struct Frustum
{
    glm::vec4 planes[6];

    static auto fromMatrix(const glm::mat4 &clip) -> Frustum
    {
        Frustum f;
        glm::vec4 row1 = {clip[0][0], clip[1][0], clip[2][0], clip[3][0]};
        glm::vec4 row2 = {clip[0][1], clip[1][1], clip[2][1], clip[3][1]};
        glm::vec4 row3 = {clip[0][2], clip[1][2], clip[2][2], clip[3][2]};
        glm::vec4 row4 = {clip[0][3], clip[1][3], clip[2][3], clip[3][3]};

        f.planes[0] = row4 + row1;
        f.planes[1] = row4 - row1;
        f.planes[2] = row4 + row2;
        f.planes[3] = row4 - row2;
        f.planes[4] = row4 + row3;
        f.planes[5] = row4 - row3;

        for (auto &p : f.planes)
        {
            float len = glm::length(glm::vec3(p));
            p /= len;
        }
        return f;
    }

    [[nodiscard]] auto cubeVisible(std::array<glm::vec3, 8> verts) const -> bool
    {
        for (int i = 0; i < 6; i++)
        {
            bool allOutside = true;
            for (int j = 0; j < 8; j++)
            {
                if (glm::dot(glm::vec3(planes[i]), verts[j]) + planes[i].w > 0.0f)
                {
                    allOutside = false;
                    break;
                }
            }
            if (allOutside)
                return false;
        }
        return true;
    }
};

OMVoxelManager::OMVoxelManager(OMRenderer *renderer, OMRendererRenderTarget *resolveTarget, OMRendererTexture *tex,
                               OMRendererTexture *texSec, std::shared_ptr<world::OMChunkManager<16>> man,
                               std::function<void(bool)> rec, OMVoxelHandler *handler,
                               std::function<uint32_t(uint32_t, uint64_t, uint64_t, uint64_t, int, int, int)> converter,
                               OMVoxelColorManager *colorman, OMRendererTexture *sunTex, OMRendererTexture *moonTex,
                               OMRendererTexture *cloudTex, std::vector<bool> cloudStats)
    : logger("OMVoxelManager", this)
{
    this->rec = rec;
    this->renderer = renderer;
    this->chunkManager = man;
    this->voxelHandler = handler;
    this->converter = converter;
    this->colorManager = colorman;
    this->sunTex = sunTex;
    this->moonTex = moonTex;
    this->cloudTex = cloudTex;
    this->cloudStats = cloudStats;

    delete compiler.handler;
    compiler.handler = voxelHandler;
    compiler.converter = converter;

    compilerPool = new OMVoxelCompilerPool(*man.get(), compiler);

    cutoutTargetMS = new OMRendererTempTarget(renderer);
    cutoutTargetMS->storeDepth = true;
    cutoutTargetMS->construct(renderer->getExtent(), samples);

    translucentTargetMS = new OMRendererTempTarget(renderer);
    translucentTargetMS->additionalTexFormats.push_back(R32Sfloat);
    translucentTargetMS->clearDepth = false;
    translucentTargetMS->construct(renderer->getExtent(), samples, true);

    cloudTargetMS = new OMRendererTempTarget(renderer);
    cloudTargetMS->additionalTexFormats.push_back(R32Sfloat);
    cloudTargetMS->clearDepth = false;
    cloudTargetMS->storeDepth = true;
    cloudTargetMS->construct(renderer->getExtent(), samples);

    cloudTarget = new OMRendererTempTarget(renderer);
    cloudTarget->additionalTexFormats.push_back(R32Sfloat);
    cloudTarget->construct(renderer->getExtent());

    cutoutTarget = new OMRendererTempTarget(renderer);
    cutoutTarget->clearDepth = false;
    cutoutTarget->construct(renderer->getExtent());
    translucentTarget = new OMRendererTempTarget(renderer);
    translucentTarget->additionalTexFormats.push_back(R32Sfloat);
    lightmap = new OMRendererTempTarget(renderer);
    lightmap->construct({16.0, 16.0}, 1, true);

    basics::OMVertexFormat voxelFormat, voxelDebug, formatComplex, simpleFormat, starFormat, cloudFormat,
        voxelFluidFormat;
    simpleFormat.nextGroup()->decideStruct();
    voxelFormat.setInstance()
        ->appendPart("voxelPos", basics::Integer)
        ->appendPart("voxelMetadata", basics::Integer)
        ->appendPart("voxelExtra", basics::Integer)
        ->appendPart("voxelExtra2", basics::Integer)
        ->appendPart("voxelExtra3", basics::Integer)
        ->nextGroup()
        ->decideStruct();

    voxelFluidFormat.setInstance()
        ->appendPart("voxelPos", basics::Integer)
        ->appendPart("voxelMetadata", basics::Integer)
        ->appendPart("voxelExtra", basics::Integer)
        ->appendPart("voxelExtra2", basics::Integer)
        ->nextGroup()
        ->decideStruct();

    voxelDebug.appendPart("voxelPos", basics::Vec3f)->nextGroup()->decideStruct();
    formatComplex.setInstance()
        ->appendPart("voxelBasics", basics::Integer)
        ->appendPart("voxelMetadata", basics::Integer)
        ->appendPart("voxelExtra", basics::Integer)
        ->appendPart("voxelOffset", basics::Vec3f)
        ->appendPart("voxelUV0", basics::Vec2f)
        ->appendPart("voxelUV1", basics::Vec2f)
        ->appendPart("voxelRotationAngle", basics::Float)
        ->appendPart("voxelRotationCenter", basics::Vec3f)
        ->appendPart("voxelSize", basics::Vec3f)
        ->appendPart("voxelRotationAngleExt1", basics::Float)
        ->appendPart("voxelRotationAngleExt2", basics::Float)
        ->nextGroup()
        ->decideStruct();

    starFormat.setInstance()
        ->appendPart("starCenter", basics::Vec3f)
        ->appendPart("starZrot", basics::Float)
        ->appendPart("starSize", basics::Float)
        ->nextGroup()
        ->decideStruct();

    cloudFormat.setInstance()->appendPart("cloudInfo", basics::Integer)->nextGroup()->decideStruct();

    pipeline =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("Camera")
            ->input(ImageSampler)
            ->inputName("inTexture")
            ->input(UniformTexelBuffer)
            ->inputName("inChunkPos")
            ->input(UniformBuffer)
            ->inputName("FogData")
            ->input(ImageSampler)
            ->inputName("inLightmap")
            ->output(cutoutTargetMS->target)
            ->samples(samples)
            ->setCullMode(renderer::common::Back)
            ->setFrontClockwise(true)
            ->shader(
                renderer->shaderManager.preprocess("core/voxel/voxel.frag.glsl", Fragment, GLSLSource, voxelFormat))
            ->shader(renderer->shaderManager.preprocess("core/voxel/voxel.vert.glsl", Vertex, GLSLSource, voxelFormat))
            ->format(voxelFormat)
            ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
            ->blend(true)
            ->depth(true, true)
            ->depthOp(LessOrEqual)
            ->buildN();

    complexPipeline = renderer->createPipeline()
                          ->input(UniformBuffer)
                          ->inputName("Camera")
                          ->input(ImageSampler)
                          ->inputName("inTexture")
                          ->input(ImageSampler)
                          ->inputName("inTextureSec")
                          ->input(UniformTexelBuffer)
                          ->inputName("inChunkPos")
                          ->input(UniformBuffer)
                          ->inputName("FogData")
                          ->input(ImageSampler)
                          ->inputName("inLightmap")
                          ->output(cutoutTargetMS->target)
                          ->samples(samples)
                          ->setCullMode(renderer::common::Back)
                          ->setFrontClockwise(true)
                          ->shader(renderer->shaderManager.preprocess("core/voxel/voxelcomplex.frag.glsl", Fragment,
                                                                      GLSLSource, formatComplex))
                          ->shader(renderer->shaderManager.preprocess("core/voxel/voxelcomplex.vert.glsl", Vertex,
                                                                      GLSLSource, formatComplex))
                          ->format(formatComplex)
                          ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
                          ->blend(true)
                          ->depth(true, true)
                          ->depthOp(LessOrEqual)
                          ->buildN();
    fluidPipeline = renderer->createPipeline()
                        ->input(UniformBuffer)
                        ->inputName("Camera")
                        ->input(ImageSampler)
                        ->inputName("inTexture")
                        ->input(ImageSampler)
                        ->inputName("inTextureSec")
                        ->input(UniformTexelBuffer)
                        ->inputName("inChunkPos")
                        ->input(UniformBuffer)
                        ->inputName("FogData")
                        ->input(ImageSampler)
                        ->inputName("inLightmap")
                        ->output(cutoutTargetMS->target)
                        ->samples(samples)
                        ->setCullMode(renderer::common::Back)
                        ->setFrontClockwise(true)
                        ->shader(renderer->shaderManager.preprocess("core/voxel/voxelfluid.frag.glsl", Fragment,
                                                                    GLSLSource, voxelFluidFormat))
                        ->shader(renderer->shaderManager.preprocess("core/voxel/voxelfluid.vert.glsl", Vertex,
                                                                    GLSLSource, voxelFluidFormat))
                        ->format(voxelFluidFormat)
                        ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
                        ->blend(true)
                        ->depth(true, true)
                        ->depthBias(true, -1.0f, -2.0f)
                        ->depthOp(Less)
                        ->buildN();
    translucentPipeline =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("Camera")
            ->input(ImageSampler)
            ->inputName("inTexture")
            ->input(UniformTexelBuffer)
            ->inputName("inChunkPos")
            ->input(UniformBuffer)
            ->inputName("FogData")
            ->input(ImageSampler)
            ->inputName("inLightmap")
            ->output(translucentTargetMS->target)
            ->samples(samples)
            ->setCullMode(renderer::common::Back)
            ->setFrontClockwise(true)
            ->shader(
                renderer->shaderManager.preprocess("core/voxel/voxel.oit.frag.glsl", Fragment, GLSLSource, voxelFormat))
            ->shader(renderer->shaderManager.preprocess("core/voxel/voxel.vert.glsl", Vertex, GLSLSource, voxelFormat))
            ->format(voxelFormat)
            ->blendFunc({One, One, One, One})
            ->blendFunc({Zero, OneMinusSrcAlpha, Zero, OneMinusSrcAlpha})
            ->blend(true)
            ->depth(true, false)
            ->depthOp(LessOrEqual)
            ->buildN();
    translucentComplexPipeline = renderer->createPipeline()
                                     ->input(UniformBuffer)
                                     ->inputName("Camera")
                                     ->input(ImageSampler)
                                     ->inputName("inTexture")
                                     ->input(ImageSampler)
                                     ->inputName("inTextureSec")
                                     ->input(UniformTexelBuffer)
                                     ->inputName("inChunkPos")
                                     ->input(UniformBuffer)
                                     ->inputName("FogData")
                                     ->input(ImageSampler)
                                     ->inputName("inLightmap")
                                     ->output(translucentTargetMS->target)
                                     ->samples(samples)
                                     ->setCullMode(renderer::common::Back)
                                     ->setFrontClockwise(true)
                                     ->shader(renderer->shaderManager.preprocess(
                                         "core/voxel/voxelcomplex.oit.frag.glsl", Fragment, GLSLSource, formatComplex))
                                     ->shader(renderer->shaderManager.preprocess("core/voxel/voxelcomplex.vert.glsl",
                                                                                 Vertex, GLSLSource, formatComplex))
                                     ->format(formatComplex)
                                     ->blendFunc({One, One, One, One})
                                     ->blendFunc({Zero, OneMinusSrcAlpha, Zero, OneMinusSrcAlpha})
                                     ->blend(true)
                                     ->depth(true, false)
                                     ->depthOp(LessOrEqual)
                                     ->buildN();
    translucentFluidPipeline = renderer->createPipeline()
                                   ->input(UniformBuffer)
                                   ->inputName("Camera")
                                   ->input(ImageSampler)
                                   ->inputName("inTexture")
                                   ->input(ImageSampler)
                                   ->inputName("inTextureSec")
                                   ->input(UniformTexelBuffer)
                                   ->inputName("inChunkPos")
                                   ->input(UniformBuffer)
                                   ->inputName("FogData")
                                   ->input(ImageSampler)
                                   ->inputName("inLightmap")
                                   ->output(translucentTargetMS->target)
                                   ->samples(samples)
                                   ->setCullMode(renderer::common::Back)
                                   ->setFrontClockwise(true)
                                   ->shader(renderer->shaderManager.preprocess("core/voxel/voxelfluid.oit.frag.glsl",
                                                                               Fragment, GLSLSource, voxelFluidFormat))
                                   ->shader(renderer->shaderManager.preprocess("core/voxel/voxelfluid.vert.glsl",
                                                                               Vertex, GLSLSource, voxelFluidFormat))
                                   ->format(voxelFluidFormat)
                                   ->blendFunc({One, One, One, One})
                                   ->blendFunc({Zero, OneMinusSrcAlpha, Zero, OneMinusSrcAlpha})
                                   ->blend(true)
                                   ->depth(true, false)
                                   ->depthBias(true, 1.0f, 2.0f)
                                   ->depthOp(Less)
                                   ->buildN();

    debugPipeline = renderer->createPipeline()
                        ->input(UniformBuffer)
                        ->inputName("Camera")
                        ->primitiveType(LineList)
                        ->setLineWidth(2.0f)
                        ->output(cutoutTargetMS->target)
                        ->samples(samples)
                        ->shader(renderer->shaderManager.preprocess("core/voxel/voxeldebug.frag.glsl", Fragment,
                                                                    GLSLSource, voxelDebug))
                        ->shader(renderer->shaderManager.preprocess("core/voxel/voxeldebug.vert.glsl", Vertex,
                                                                    GLSLSource, voxelDebug))
                        ->format(voxelDebug)
                        ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
                        ->blend(true)
                        ->depth(true, true)
                        ->depthOp(Less)
                        ->buildN();

    skyDiscPipeline = renderer->createPipeline()
                          ->input(UniformBuffer)
                          ->inputName("Camera")
                          ->input(UniformBuffer)
                          ->inputName("SkyDiscData")
                          ->output(cutoutTargetMS->target)
                          ->samples(samples)
                          ->primitiveType(TriangleFan)
                          ->shader(renderer->shaderManager.preprocess("core/voxel/skydisc.frag.glsl", Fragment,
                                                                      GLSLSource, simpleFormat))
                          ->shader(renderer->shaderManager.preprocess("core/voxel/skydisc.vert.glsl", Vertex,
                                                                      GLSLSource, simpleFormat))
                          ->format(simpleFormat)
                          ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
                          ->blend(true)
                          ->depth(false, true)
                          ->depthOp(Greater)
                          ->buildN();

    sunPipeline =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("Camera")
            ->input(UniformBuffer)
            ->inputName("SunRiseData")
            ->input(ImageSampler)
            ->inputName("inTexture")
            ->output(cutoutTargetMS->target)
            ->samples(samples)
            ->shader(renderer->shaderManager.preprocess("core/voxel/sun.frag.glsl", Fragment, GLSLSource, simpleFormat))
            ->shader(renderer->shaderManager.preprocess("core/voxel/sun.vert.glsl", Vertex, GLSLSource, simpleFormat))
            ->format(simpleFormat)
            ->blendFunc({One, One, One, One})
            ->blend(true)
            ->depth(false, true)
            ->depthOp(Greater)
            ->buildN();

    moonPipeline =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("Camera")
            ->input(UniformBuffer)
            ->inputName("MoonData")
            ->input(ImageSampler)
            ->inputName("inTexture")
            ->output(cutoutTargetMS->target)
            ->samples(samples)
            ->shader(
                renderer->shaderManager.preprocess("core/voxel/moon.frag.glsl", Fragment, GLSLSource, simpleFormat))
            ->shader(renderer->shaderManager.preprocess("core/voxel/moon.vert.glsl", Vertex, GLSLSource, simpleFormat))
            ->format(simpleFormat)
            ->blendFunc({SrcAlpha, One, SrcAlpha, One})
            ->blend(true)
            ->depth(false, true)
            ->depthOp(Greater)
            ->buildN();

    starPipeline =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("Camera")
            ->input(UniformBuffer)
            ->inputName("StarData")
            ->output(cutoutTargetMS->target)
            ->samples(samples)
            ->shader(renderer->shaderManager.preprocess("core/voxel/star.frag.glsl", Fragment, GLSLSource, starFormat))
            ->shader(renderer->shaderManager.preprocess("core/voxel/star.vert.glsl", Vertex, GLSLSource, starFormat))
            ->format(starFormat)
            ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
            ->blend(true)
            ->depth(false, true)
            ->depthOp(Greater)
            ->buildN();

    sunrisePipeline = renderer->createPipeline()
                          ->input(UniformBuffer)
                          ->inputName("Camera")
                          ->input(UniformBuffer)
                          ->inputName("SunRiseData")
                          ->output(cutoutTargetMS->target)
                          ->samples(samples)
                          ->primitiveType(TriangleFan)
                          ->shader(renderer->shaderManager.preprocess("core/voxel/sunrise.frag.glsl", Fragment,
                                                                      GLSLSource, simpleFormat))
                          ->shader(renderer->shaderManager.preprocess("core/voxel/sunrise.vert.glsl", Vertex,
                                                                      GLSLSource, simpleFormat))
                          ->format(simpleFormat)
                          ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
                          ->blend(true)
                          ->depth(false, true)
                          ->depthOp(Greater)
                          ->buildN();

    cloudPipeline = renderer->createPipeline()
                        ->input(UniformBuffer)
                        ->inputName("Camera")
                        ->input(UniformBuffer)
                        ->inputName("CloudData")
                        ->input(UniformBuffer)
                        ->inputName("FogData")
                        ->output(cloudTargetMS->target)
                        ->samples(samples)
                        ->shader(renderer->shaderManager.preprocess("core/voxel/voxelcloud.frag.glsl", Fragment,
                                                                    GLSLSource, cloudFormat))
                        ->shader(renderer->shaderManager.preprocess("core/voxel/voxelcloud.vert.glsl", Vertex,
                                                                    GLSLSource, cloudFormat))
                        ->format(cloudFormat)
                        ->blendFunc({One, Zero, One, Zero})
                        ->blendFunc({OneMinusSrcAlpha, Zero, OneMinusSrcAlpha, Zero})
                        ->blend(true)
                        ->depth(true, true)
                        ->depthOp(LessOrEqual)
                        ->buildN();

    cloudComposePipeline =
        renderer->createPipeline()
            ->input(ImageSampler)
            ->inputName("inTexture")
            ->input(ImageSampler)
            ->inputName("inTextureReveal")
            ->output(translucentTargetMS->target)
            ->samples(samples)
            ->shader(
                renderer->shaderManager.preprocess("core/voxel/cloud.frag.glsl", Fragment, GLSLSource, simpleFormat))
            ->shader(renderer->shaderManager.preprocess("core/voxel/cloud.vert.glsl", Vertex, GLSLSource, simpleFormat))
            ->format(simpleFormat)
            ->blendFunc({One, One, One, One})
            ->blendFunc({Zero, SrcAlpha, Zero, SrcAlpha})
            ->blend(true)
            ->depth(true, false)
            ->depthOp(Always)
            ->buildN();

    lightmapPipeline = renderer->createPipeline()
                           ->input(UniformBuffer)
                           ->inputName("LightmapInfo")
                           ->output(lightmap->target)
                           ->samples(1)
                           ->shader(renderer->shaderManager.preprocess("core/voxel/lightmap.frag.glsl", Fragment,
                                                                       GLSLSource, simpleFormat))
                           ->shader(renderer->shaderManager.preprocess("core/voxel/lightmap.vert.glsl", Vertex,
                                                                       GLSLSource, simpleFormat))
                           ->format(simpleFormat)
                           ->blendFunc({One, One, One, One})
                           ->blend(false)
                           ->depth(false, true)
                           ->buildN();

    skyPipeline =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("SkyDiscData")
            ->output(cutoutTargetMS->target)
            ->samples(samples)
            ->shader(renderer->shaderManager.preprocess("core/voxel/sky.frag.glsl", Fragment, GLSLSource, simpleFormat))
            ->shader(renderer->shaderManager.preprocess("core/voxel/sky.vert.glsl", Vertex, GLSLSource, simpleFormat))
            ->format(simpleFormat)
            ->blendFunc({One, One, One, One})
            ->blend(false)
            ->depth(false, true)
            ->buildN();

    composePipeline = renderer->createPipeline()
                          ->input(ImageSampler)
                          ->inputName("inTextureCutout")
                          ->input(ImageSampler)
                          ->inputName("inTextureTranslucent")
                          ->input(ImageSampler)
                          ->inputName("inTextureTranslucentReveal")
                          ->output(resolveTarget)
                          ->samples(1)
                          ->shader(renderer->shaderManager.preprocess("core/voxel/voxelcompose.frag.glsl", Fragment,
                                                                      GLSLSource, simpleFormat))
                          ->shader(renderer->shaderManager.preprocess("core/voxel/voxelcompose.vert.glsl", Vertex,
                                                                      GLSLSource, simpleFormat))
                          ->format(simpleFormat)
                          ->blendFunc({One, One, One, One})
                          ->blend(false)
                          ->depth(false, false)
                          ->buildN();

    voxelLayer = new OMVoxelLayer<OMVoxel>(renderer);
    voxelComplexLayer = new OMVoxelLayer<OMVoxelComplex>(renderer);
    voxelTranslucentLayer = new OMVoxelLayer<OMVoxel>(renderer);
    voxelTranslucentComplexLayer = new OMVoxelLayer<OMVoxelComplex>(renderer);
    voxelFluidLayer = new OMVoxelLayer<OMVoxelFluid>(renderer);
    voxelTranslucentFluidLayer = new OMVoxelLayer<OMVoxelFluid>(renderer);

    chunkoffs = renderer->allocateBuffer(UniformTexel, 3 * sizeof(float));
    debugoffs = renderer->allocateBuffer(VertexData, 12 * 2 * 3 * sizeof(float));
    skydisc = renderer->allocateBuffer(Uniform, sizeof(OMVoxelSkyDisc));
    fogdata = renderer->allocateBuffer(Uniform, sizeof(float) * 5);
    cloudFogdata = renderer->allocateBuffer(Uniform, sizeof(float) * 5);
    sunrise = renderer->allocateBuffer(Uniform, sizeof(OMVoxelSunrise));
    moonData = renderer->allocateBuffer(Uniform, sizeof(OMVoxelMoon));
    cloudData = renderer->allocateBuffer(Uniform, sizeof(OMVoxelCloud));

    {
        auto p = buildVoxelCloud();
        cloudBuffer = renderer->allocateBuffer(InstanceData, sizeof(uint32_t) * p.size());
        cloudBuffer->updateData(p.data());
    }

    lightmapData = renderer->allocateBuffer(Uniform, sizeof(OMVoxelLightMap));

    textureAtlas = tex;
    textureAtlasSecondary = texSec;

    std::mt19937 gen(1029);
    std::uniform_real_distribution<> distrib(0.0f, 1.0f);

    std::vector<float> starData = {};
    for (int i = 0; i < 16; ++i)
    {
        starData.push_back(distrib(gen));
    }
    starData.clear();

    for (int i = 0; i < 1500; i++)
    {
        auto x = (distrib(gen) * 2.0f) - 1.0f;
        auto y = (distrib(gen) * 2.0f) - 1.0f;
        auto z = (distrib(gen) * 2.0f) - 1.0f;

        float starSize = 0.15f + (distrib(gen) * 0.1f);

        float lengthSq = sqrt(x * x + y * y + z * z);
        if (lengthSq > 0.010000001f && lengthSq < 1.0f)
        {
            auto center = glm::normalize(glm::vec3(x, y, z)) * glm::vec3(100.0f);
            auto zrot = distrib(gen) * 3.1415927410125732 * 2.0;

            starData.emplace_back(center.x);
            starData.emplace_back(center.y);
            starData.emplace_back(center.z);
            starData.emplace_back(zrot);
            starData.emplace_back(starSize);
        }
    }
    starBuffer = renderer->allocateBuffer(InstanceData, starData.size() * sizeof(float));
    starBuffer->updateData(starData.data());

    starBaseData = renderer->allocateBuffer(Uniform, sizeof(float) * 2);

    pipeline->bindInput(1, textureAtlas);
    pipeline->bindInput(2, chunkoffs);
    pipeline->bindInput(3, fogdata);
    pipeline->bindInput(4, lightmap->colorTexture);
    complexPipeline->bindInput(1, textureAtlas);
    complexPipeline->bindInput(2, textureAtlasSecondary);
    complexPipeline->bindInput(3, chunkoffs);
    complexPipeline->bindInput(4, fogdata);
    complexPipeline->bindInput(5, lightmap->colorTexture);
    fluidPipeline->bindInput(1, textureAtlas);
    fluidPipeline->bindInput(2, textureAtlasSecondary);
    fluidPipeline->bindInput(3, chunkoffs);
    fluidPipeline->bindInput(4, fogdata);
    fluidPipeline->bindInput(5, lightmap->colorTexture);
    translucentPipeline->bindInput(1, textureAtlas);
    translucentPipeline->bindInput(2, chunkoffs);
    translucentPipeline->bindInput(3, fogdata);
    translucentPipeline->bindInput(4, lightmap->colorTexture);
    translucentComplexPipeline->bindInput(1, textureAtlas);
    translucentComplexPipeline->bindInput(2, textureAtlasSecondary);
    translucentComplexPipeline->bindInput(3, chunkoffs);
    translucentComplexPipeline->bindInput(4, fogdata);
    translucentComplexPipeline->bindInput(5, lightmap->colorTexture);
    translucentFluidPipeline->bindInput(1, textureAtlas);
    translucentFluidPipeline->bindInput(2, textureAtlasSecondary);
    translucentFluidPipeline->bindInput(3, chunkoffs);
    translucentFluidPipeline->bindInput(4, fogdata);
    translucentFluidPipeline->bindInput(5, lightmap->colorTexture);
    skyDiscPipeline->bindInput(1, skydisc);
    lightmapPipeline->bindInput(0, lightmapData);
    skyPipeline->bindInput(0, skydisc);
    sunrisePipeline->bindInput(1, sunrise);
    sunPipeline->bindInput(1, sunrise);
    sunPipeline->bindInput(2, sunTex);
    moonPipeline->bindInput(1, moonData);
    moonPipeline->bindInput(2, moonTex);
    starPipeline->bindInput(1, starBaseData);
    cloudPipeline->bindInput(1, cloudData);
    cloudPipeline->bindInput(2, cloudFogdata);
}
OMVoxelManager::~OMVoxelManager()
{
    delete skyPipeline;
    delete lightmap;
    delete lightmapPipeline;
    delete lightmapData;
    delete fogdata;
    delete cloudFogdata;
    delete compilerPool;
    delete voxelLayer;
    delete voxelComplexLayer;
    delete voxelTranslucentLayer;
    delete voxelTranslucentComplexLayer;
    delete voxelFluidLayer;
    delete voxelTranslucentFluidLayer;
    delete chunkoffs;
    delete debugoffs;
    delete skydisc;
    delete sunrise;
    delete moonData;
    delete pipeline;
    delete complexPipeline;
    delete fluidPipeline;
    delete translucentPipeline;
    delete translucentComplexPipeline;
    delete translucentFluidPipeline;
    delete debugPipeline;
    delete composePipeline;
    delete translucentTargetMS;
    delete cutoutTargetMS;
    delete cloudTargetMS;
    delete cutoutTarget;
    delete translucentTarget;
    delete cloudTarget;
    delete skyDiscPipeline;
    delete sunrisePipeline;
    delete sunPipeline;
    delete moonPipeline;
    delete starBuffer;
    delete starPipeline;
    delete starBaseData;
    delete cloudData;
    delete cloudPipeline;
    delete cloudBuffer;
    delete cloudComposePipeline;
}

void OMVoxelManager::unloadChunk(int i)
{
    std::vector<OMVoxel> m = {}, tm = {};
    std::vector<OMVoxelComplex> cm = {}, tcm = {};
    std::vector<OMVoxelFluid> fm = {}, tfm = {};
    voxelLayer->loadData(i, m);
    voxelComplexLayer->loadData(i, cm);
    voxelTranslucentLayer->loadData(i, tm);
    voxelTranslucentComplexLayer->loadData(i, tcm);
    voxelFluidLayer->loadData(i, fm);
    voxelTranslucentFluidLayer->loadData(i, tfm);
}

auto srgbToLinear(const glm::vec3 &c) -> glm::vec3
{
    glm::vec3 lo = c / 12.92f;
    glm::vec3 hi = glm::pow((c + 0.055f) / 1.055f, glm::vec3(2.4f));
    glm::vec3 s = glm::step(glm::vec3(0.04045f), c);
    return glm::mix(lo, hi, s);
}

auto srgbToLinear(const glm::vec4 &cp) -> glm::vec4
{
    auto c = glm::vec3(cp.x, cp.y, cp.z);
    glm::vec3 lo = c / 12.92f;
    glm::vec3 hi = glm::pow((c + 0.055f) / 1.055f, glm::vec3(2.4f));
    glm::vec3 s = glm::step(glm::vec3(0.04045f), c);
    auto r = glm::mix(lo, hi, s);
    return {r.x, r.y, r.z, cp.w};
}

auto OMVoxelManager::updateColor() -> void
{
    if (colorManager->isDirty())
    {
        OMVoxelSkyDisc disc = {srgbToLinear(colorManager->getSkyDiscColor()), colorManager->getSkyDiskRange(),
                               srgbToLinear(colorManager->getFogColor()), colorManager->getSkyDiskHeight()};
        skydisc->updateData(&disc);
        OMVoxelSunrise ris = {srgbToLinear(colorManager->getSunriseColor()), colorManager->getSunAngle()};
        sunrise->updateData(&ris);
        auto fg = srgbToLinear(colorManager->getFogColor());
        std::array<float, 5> d = {colorManager->getFogRange().x, colorManager->getFogRange().y, fg.r, fg.g, fg.b};
        fogdata->updateData(d.data());

        std::array<float, 5> d2 = {colorManager->getFogRange().x * 3, colorManager->getFogRange().y * 3, fg.r, fg.g,
                                   fg.b};
        cloudFogdata->updateData(d2.data());
        OMVoxelMoon m = {colorManager->getMoonAngle(), colorManager->getMoonPhase()};
        moonData->updateData(&m);
        starBaseData->updateData(
            std::array<float, 2>{colorManager->getStarRotation(), colorManager->getStarOpacity()}.data());

        OMVoxelLightMap dayData = {colorManager->getSkyFactor(),
                                   colorManager->getBlockFactor(),
                                   colorManager->getNightVisionFactor(),
                                   colorManager->getDarknessScale(),
                                   colorManager->getBossOverlayWorldDarkeningFactor(),
                                   colorManager->getBrightnessFactor(),
                                   0,
                                   0,
                                   colorManager->getBlockTint(),
                                   0,
                                   srgbToLinear(colorManager->getSkyLightColor()),
                                   0,
                                   srgbToLinear(colorManager->getAmbientColor()),
                                   0,
                                   srgbToLinear(colorManager->getNightVisionColor())};
        lightmapData->updateData(&dayData);
        colorManager->solveDirty();
    }
}
auto OMVoxelManager::buildVoxelCloud() -> std::vector<uint32_t>
{
    auto ex = [&](uint8_t x, uint8_t y) -> bool { return cloudStats[static_cast<int>(y << 8) | x]; };
    std::vector<uint32_t> da;
    for (int x = 0; x <= 255; ++x)
    {
        for (int y = 0; y <= 255; ++y)
        {
            if (ex(x, y))
            {
                auto east = ex(x + 1, y);
                auto west = ex(x - 1, y);
                auto north = ex(x, y + 1);
                auto south = ex(x, y - 1);

                da.push_back(east << 20 | west << 19 | north << 18 | south << 17 | ((x & 0xff) << 8) | (y & 0xff));
            }
        }
    }
    return da;
}

auto OMVoxelManager::fetchLayerState() -> std::array<std::pair<uint32_t, uint32_t>, 6>
{
    return {{
        {voxelLayer->buf()->usedSize(), voxelLayer->buf()->totalSize},
        {voxelComplexLayer->buf()->usedSize(), voxelComplexLayer->buf()->totalSize},
        {voxelFluidLayer->buf()->usedSize(), voxelFluidLayer->buf()->totalSize},
        {voxelTranslucentLayer->buf()->usedSize(), voxelTranslucentLayer->buf()->totalSize},
        {voxelTranslucentComplexLayer->buf()->usedSize(), voxelTranslucentComplexLayer->buf()->totalSize},
        {voxelTranslucentFluidLayer->buf()->usedSize(), voxelTranslucentFluidLayer->buf()->totalSize},
    }};
}

auto OMVoxelManager::hashLayerState() -> std::array<uint32_t, 12>
{
    return {
        voxelLayer->buf()->usedSize(),
        voxelLayer->buf()->totalSize,
        voxelComplexLayer->buf()->usedSize(),
        voxelComplexLayer->buf()->totalSize,
        voxelFluidLayer->buf()->usedSize(),
        voxelFluidLayer->buf()->totalSize,
        voxelTranslucentLayer->buf()->usedSize(),
        voxelTranslucentLayer->buf()->totalSize,
        voxelTranslucentComplexLayer->buf()->usedSize(),
        voxelTranslucentComplexLayer->buf()->totalSize,
        voxelTranslucentFluidLayer->buf()->usedSize(),
        voxelTranslucentFluidLayer->buf()->totalSize,
    };
}

auto OMVoxelManager::fetchChunkState() -> std::pair<uint32_t, uint32_t>
{
    uint32_t total = 0, nonfree = 0;
    chunkManager->withChunks([&](std::vector<std::optional<world::OMChunk<16>>> &chunks) -> void {
        for (const auto &c : chunks)
        {
            ++total;
            if (c->visible)
            {
                ++nonfree;
            }
        }
    });
    return {total, nonfree};
}

static float tt = 0.0f;
static auto lastp = std::chrono::steady_clock::now();

auto OMVoxelManager::update(basics::OMCamera &camera) -> void
{
    auto cc = camera.getPosRaw();

    auto cur = std::chrono::steady_clock::now();
    tt += std::chrono::duration_cast<std::chrono::milliseconds>(cur - lastp).count() / 1000.0f * 3.0f;
    tt = std::fmod(tt, 256 * 12);
    lastp = cur;

    auto cloudc = colorManager->getCloudColor();

    cloudData->updateData(std::array<OMVoxelCloud, 1>{{{{cc.getModX(256 * 12) + tt, cc.getY(), cc.getModZ(256 * 12)},
                                                        cloudc.a,
                                                        srgbToLinear(glm::vec3(cloudc.r, cloudc.g, cloudc.b))}}}
                              .data());

    auto pp = basics::OMPosition<16, int64_t, float>(cc.chunkx, cc.chunky, cc.chunkz);
    auto pp2 = basics::OMPosition<16, int64_t, float>(cc.chunkx + 1, cc.chunky, cc.chunkz);
    auto pp3 = basics::OMPosition<16, int64_t, float>(cc.chunkx, cc.chunky + 1, cc.chunkz);
    auto pp4 = basics::OMPosition<16, int64_t, float>(cc.chunkx, cc.chunky, cc.chunkz + 1);
    auto pp5 = basics::OMPosition<16, int64_t, float>(cc.chunkx + 1, cc.chunky + 1, cc.chunkz);
    auto pp6 = basics::OMPosition<16, int64_t, float>(cc.chunkx + 1, cc.chunky, cc.chunkz + 1);
    auto pp7 = basics::OMPosition<16, int64_t, float>(cc.chunkx, cc.chunky + 1, cc.chunkz + 1);
    auto pp8 = basics::OMPosition<16, int64_t, float>(cc.chunkx + 1, cc.chunky + 1, cc.chunkz + 1);

    debugoffs->updateData(std::array<glm::vec3, 2 * 12>{
        {
            pp - camera.getPosRaw(),  pp2 - camera.getPosRaw(), pp - camera.getPosRaw(),  pp3 - camera.getPosRaw(),
            pp - camera.getPosRaw(),  pp4 - camera.getPosRaw(), pp8 - camera.getPosRaw(), pp5 - camera.getPosRaw(),
            pp8 - camera.getPosRaw(), pp6 - camera.getPosRaw(), pp8 - camera.getPosRaw(), pp7 - camera.getPosRaw(),
            pp2 - camera.getPosRaw(), pp5 - camera.getPosRaw(), pp2 - camera.getPosRaw(), pp6 - camera.getPosRaw(),
            pp3 - camera.getPosRaw(), pp5 - camera.getPosRaw(), pp3 - camera.getPosRaw(), pp7 - camera.getPosRaw(),
            pp4 - camera.getPosRaw(), pp6 - camera.getPosRaw(), pp4 - camera.getPosRaw(), pp7 - camera.getPosRaw(),
        }}.data());

    updateColor();

    if (chunkManager->numChunks())
    {
        if (chunkOffsetCache.size() != chunkManager->numChunks())
        {
            chunkOffsetCache.resize(chunkManager->numChunks());
        }

        auto layerState = hashLayerState();
        chunkManager->withChunks([&](std::vector<std::optional<world::OMChunk<16>>> &chunks) -> void {
            int i = 0;
            for (auto &ochk : chunks)
            {
                if (!ochk.has_value())
                {
                    chunkOffsetCache[i] = glm::vec3{INFINITY};
                }
                else
                {
                    const auto &chk = ochk.value();
                    auto pp = basics::OMPosition<16, int64_t, float>();
                    pp.chunkx = chk.chunkx;
                    pp.chunky = chk.chunky;
                    pp.chunkz = chk.chunkz;
                    chunkOffsetCache[i] = pp - camera.getPosRaw();
                }
                ++i;
            }
        });

        auto frusturm = Frustum::fromMatrix(camera.fetchProjMat() * camera.fetchViewMat());
        chunkManager->withChunks([&](std::vector<std::optional<world::OMChunk<16>>> &chunks) -> void {
            int i = 0;
            for (auto &ochk : chunks)
            {
                if (!ochk.has_value())
                {
                    compilerPool->dropCache(i);
                    unloadChunk(i);
                }
                else
                {
                    auto &chk = ochk.value();

                    auto cc = basics::OMPosition<16, int64_t, float>();
                    cc.chunkx = chk.chunkx;
                    cc.chunky = chk.chunky;
                    cc.chunkz = chk.chunkz;
                    cc.localx = 0.0f;
                    cc.localy = 0.0f;
                    cc.localz = 0.0f;

                    auto pp0 =
                        basics::OMPosition<16, int64_t, float>(cc.chunkx, cc.chunky, cc.chunkz) - camera.getPosRaw();
                    auto pp1 = basics::OMPosition<16, int64_t, float>(cc.chunkx + 1, cc.chunky + 1, cc.chunkz + 1) -
                               camera.getPosRaw();

                    auto visible = frusturm.cubeVisible(std::array<glm::vec3, 8>{{
                        {pp0.x, pp0.y, pp0.z},
                        {pp1.x, pp0.y, pp0.z},
                        {pp0.x, pp1.y, pp0.z},
                        {pp1.x, pp1.y, pp0.z},
                        {pp0.x, pp0.y, pp1.z},
                        {pp1.x, pp0.y, pp1.z},
                        {pp0.x, pp1.y, pp1.z},
                        {pp1.x, pp1.y, pp1.z},
                    }});

                    if (chk.visible && !visible)
                    {
                        unloadChunk(i);
                    }
                    else if (chk.visible && visible)
                    {
                        if (chk.isDirty())
                        {
                            compilerPool->compile(i, false);
                            chk.solveDirty();
                        }
                    }
                    else if (!chk.visible && visible)
                    {
                        compilerPool->compile(i, true);
                        chk.solveDirty();
                    }

                    chk.visible = visible;
                }
                ++i;
            }
        });

        if (chunkOffsetCache.size() * sizeof(glm::vec3) > chunkoffs->length)
        {
            delete chunkoffs;
            chunkoffs = renderer->allocateBuffer(UniformTexel, chunkManager->numChunks() * 3 * sizeof(float) * 2);
            pipeline->bindInput(2, chunkoffs);
            complexPipeline->bindInput(3, chunkoffs);
            fluidPipeline->bindInput(3, chunkoffs);
            translucentPipeline->bindInput(2, chunkoffs);
            translucentComplexPipeline->bindInput(3, chunkoffs);
            translucentFluidPipeline->bindInput(3, chunkoffs);
        }

        chunkoffs->synced = true;
        chunkoffs->updateDataPart(chunkOffsetCache.data(), 0, chunkOffsetCache.size() * sizeof(glm::vec3));

        compilerPool->upload(voxelLayer, voxelComplexLayer, voxelFluidLayer, voxelTranslucentLayer,
                             voxelTranslucentComplexLayer, voxelTranslucentFluidLayer);

        if (layerState != hashLayerState())
        {
            rec(false);
        }
    }
}

auto OMVoxelManager::submit(OMRendererTask *task, OMRendererTempTarget *resolveTarget, bool r) -> OMRendererTask *
{
    if (r)
    {
        cutoutTargetMS->storeDepth = true;
        cutoutTargetMS->construct(renderer->getExtent(), samples);
        translucentTargetMS->clearDepth = false;
        translucentTargetMS->constructWithDepth(cutoutTargetMS->depthTexture, renderer->getExtent(), samples, true);
        cutoutTarget->construct(renderer->getExtent());
        translucentTarget->construct(renderer->getExtent(), 1, true);
        cloudTargetMS->clearDepth = false;
        cloudTargetMS->constructWithDepth(cutoutTargetMS->depthTexture, renderer->getExtent(), samples);
        cloudTarget->construct(renderer->getExtent());
        composePipeline->bindInput(0, (samples == 1 ? cutoutTargetMS : cutoutTarget)->colorTexture);
        composePipeline->bindInput(1, (samples == 1 ? translucentTargetMS : translucentTarget)->colorTexture);
        composePipeline->bindInput(2, (samples == 1 ? translucentTargetMS : translucentTarget)->additionalTex[0]);
        cloudComposePipeline->bindInput(0, (samples == 1 ? cloudTargetMS : cloudTarget)->colorTexture);
        cloudComposePipeline->bindInput(1, (samples == 1 ? cloudTargetMS : cloudTarget)->additionalTex[0]);
    }

    auto tsk = task->target(lightmap->target)
                   ->beginDebugTag("environment")
                   ->pipeline(lightmapPipeline)
                   ->drawN(6)
                   ->clearColor({0.0f, 0.0f, 0.0f, 0.0f})
                   ->clearDepth(1.0f)
                   ->target(cutoutTargetMS->target)
                   ->pipeline(skyPipeline)
                   ->drawN(6)
                   ->pipeline(skyDiscPipeline)
                   ->drawN(10)
                   ->pipeline(sunrisePipeline)
                   ->drawN(18)
                   ->pipeline(sunPipeline)
                   ->drawN(6)
                   ->pipeline(moonPipeline)
                   ->drawN(6)
                   ->pipeline(starPipeline)
                   ->vertexBuffer({starBuffer})
                   ->drawInstanceN(6, starBuffer->length / sizeof(float) / 5)
                   ->endDebugTag()
                   ->beginDebugTag("cutout/opaque chunks")
                   ->pipeline(pipeline)
                   ->vertexBuffer({voxelLayer->buf()->buffer})
                   ->drawInstanceN(6, voxelLayer->buf()->usedSize() / sizeof(OMVoxel))
                   ->pipeline(complexPipeline)
                   ->vertexBuffer({voxelComplexLayer->buf()->buffer})
                   ->drawInstanceN(6, voxelComplexLayer->buf()->usedSize() / sizeof(OMVoxelComplex))
                   ->pipeline(fluidPipeline)
                   ->vertexBuffer({voxelFluidLayer->buf()->buffer})
                   ->drawInstanceN(6, voxelFluidLayer->buf()->usedSize() / sizeof(OMVoxelFluid))
                   ->pipeline(debugPipeline)
                   ->vertexBuffer({debugoffs})
                   ->drawN(2 * 12)
                   ->endDebugTag();

    if (samples != 1)
    {
        tsk->resolve(cutoutTarget->target);
    }

    tsk->beginDebugTag("clouds")
        ->clearColor(glm::vec4(0.0), 0)
        ->clearColor(glm::vec4(1.0), 1)
        ->target(cloudTargetMS->target)
        ->pipeline(cloudPipeline)
        ->vertexBuffer({cloudBuffer})
        ->drawInstanceN(36, cloudBuffer->length / sizeof(uint32_t))
        ->endDebugTag();

    if (samples != 1)
    {
        tsk->resolve(cloudTarget->target);
    }

    tsk->beginDebugTag("translucent chunks")
        ->clearColor(glm::vec4(0.0), 0)
        ->clearColor(glm::vec4(1.0), 1)
        ->target(translucentTargetMS->target)
        ->pipeline(cloudComposePipeline)
        ->drawN(6)
        ->pipeline(translucentPipeline)
        ->vertexBuffer({voxelTranslucentLayer->buf()->buffer})
        ->drawInstanceN(6, voxelTranslucentLayer->buf()->usedSize() / sizeof(OMVoxel))
        ->pipeline(translucentComplexPipeline)
        ->vertexBuffer({voxelTranslucentComplexLayer->buf()->buffer})
        ->drawInstanceN(6, voxelTranslucentComplexLayer->buf()->usedSize() / sizeof(OMVoxelComplex))
        ->pipeline(translucentFluidPipeline)
        ->vertexBuffer({voxelTranslucentFluidLayer->buf()->buffer})
        ->drawInstanceN(6, voxelTranslucentFluidLayer->buf()->usedSize() / sizeof(OMVoxelFluid))
        ->endDebugTag();

    if (samples != 1)
    {
        tsk->resolve(translucentTarget->target);
    }
    return tsk->target(resolveTarget->target)->pipeline(composePipeline)->drawN(6);
}

void OMVoxelManager::bindCameraBuffer(OMRendererBuffer *cameraBuffer)
{
    pipeline->bindInput(0, cameraBuffer);
    debugPipeline->bindInput(0, cameraBuffer);
    complexPipeline->bindInput(0, cameraBuffer);
    fluidPipeline->bindInput(0, cameraBuffer);
    translucentPipeline->bindInput(0, cameraBuffer);
    translucentComplexPipeline->bindInput(0, cameraBuffer);
    translucentFluidPipeline->bindInput(0, cameraBuffer);
    skyDiscPipeline->bindInput(0, cameraBuffer);
    sunrisePipeline->bindInput(0, cameraBuffer);
    sunPipeline->bindInput(0, cameraBuffer);
    moonPipeline->bindInput(0, cameraBuffer);
    starPipeline->bindInput(0, cameraBuffer);
    cloudPipeline->bindInput(0, cameraBuffer);
}
} // namespace openminecraft::renderer::common::wrap
