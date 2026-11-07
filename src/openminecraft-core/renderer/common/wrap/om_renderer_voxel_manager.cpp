#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_segbuf.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_segbuf.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include <algorithm>
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

OMVoxelManager::OMVoxelManager(
    OMRenderer *renderer, OMRendererRenderTarget *resolveTarget, OMRendererTexture *tex, OMRendererTexture *texSec,
    std::shared_ptr<world::OMChunkManager<16>> man, std::function<void(bool)> recordTask, OMVoxelHandler *handler,
    std::function<uint32_t(uint32_t, uint64_t, uint64_t, uint64_t, int, int, int)> modelFetcher,
    OMVoxelColorManager *colorman, OMRendererTexture *sunTex, OMRendererTexture *moonTex, OMRendererTexture *cloudTex,
    std::vector<bool> cloudStats, std::shared_ptr<OMChunkPaletteProvider> paletteProvider)
    : logger("OMVoxelManager", this)
{
    this->recordTask = recordTask;
    this->renderer = renderer;
    this->chunkManager = man;
    this->voxelHandler = handler;
    this->paletteProvider = paletteProvider;
    this->compiler.paletteProvider = this->paletteProvider;
    this->modelFetcher = modelFetcher;
    this->colorManager = colorman;
    this->sunTex = sunTex;
    this->moonTex = moonTex;
    this->cloudTex = cloudTex;
    this->cloudStats = cloudStats;

    delete compiler.handler;
    compiler.handler = voxelHandler;
    compiler.modelFetcher = modelFetcher;

    compilerPool = new OMVoxelCompilerPool(*man.get(), compiler, 12);

    cutoutTargetMS = new OMRendererTempTarget(renderer);
    cutoutTargetMS->setStoreDepth(true);
    cutoutTargetMS->construct(renderer->getExtent(), samples);

    translucentTargetMS = new OMRendererTempTarget(renderer);
    translucentTargetMS->additionalTexFormats.push_back(R32Sfloat);
    translucentTargetMS->setClearDepth(false);
    translucentTargetMS->construct(renderer->getExtent(), samples, true);

    cloudTargetMS = new OMRendererTempTarget(renderer);
    cloudTargetMS->additionalTexFormats.push_back(R32Sfloat);
    cloudTargetMS->setClearDepth(false);
    cloudTargetMS->setStoreDepth(true);
    cloudTargetMS->construct(renderer->getExtent(), samples);

    cloudTarget = new OMRendererTempTarget(renderer);
    cloudTarget->additionalTexFormats.push_back(R32Sfloat);
    cloudTarget->construct(renderer->getExtent());

    cutoutTarget = new OMRendererTempTarget(renderer);
    cutoutTarget->setClearDepth(false);
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
        ->appendPart("voxelType", basics::Integer)
        ->nextGroup()
        ->decideStruct();

    voxelFluidFormat.setInstance()
        ->appendPart("voxelPos", basics::Integer)
        ->appendPart("voxelMetadata", basics::Integer)
        ->appendPart("voxelExtra", basics::Integer)
        ->appendPart("voxelExtra2", basics::Integer)
        ->appendPart("voxelType", basics::Integer)
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
        ->appendPart("voxelType", basics::Integer)
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

    chunkOffsetBuffer = renderer->allocateBuffer(UniformTexel, 3 * sizeof(float));
    debugOffsetBuffer = renderer->allocateBuffer(VertexData, 12 * 2 * 3 * sizeof(float));
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
    pipeline->bindInput(2, chunkOffsetBuffer);
    pipeline->bindInput(3, fogdata);
    pipeline->bindInput(4, lightmap->colorTexture);

    complexPipeline->bindInput(1, textureAtlas);
    complexPipeline->bindInput(2, textureAtlasSecondary);
    complexPipeline->bindInput(3, chunkOffsetBuffer);
    complexPipeline->bindInput(4, fogdata);
    complexPipeline->bindInput(5, lightmap->colorTexture);

    fluidPipeline->bindInput(1, textureAtlas);
    fluidPipeline->bindInput(2, textureAtlasSecondary);
    fluidPipeline->bindInput(3, chunkOffsetBuffer);
    fluidPipeline->bindInput(4, fogdata);
    fluidPipeline->bindInput(5, lightmap->colorTexture);

    translucentPipeline->bindInput(1, textureAtlas);
    translucentPipeline->bindInput(2, chunkOffsetBuffer);
    translucentPipeline->bindInput(3, fogdata);
    translucentPipeline->bindInput(4, lightmap->colorTexture);

    translucentComplexPipeline->bindInput(1, textureAtlas);
    translucentComplexPipeline->bindInput(2, textureAtlasSecondary);
    translucentComplexPipeline->bindInput(3, chunkOffsetBuffer);
    translucentComplexPipeline->bindInput(4, fogdata);
    translucentComplexPipeline->bindInput(5, lightmap->colorTexture);

    translucentFluidPipeline->bindInput(1, textureAtlas);
    translucentFluidPipeline->bindInput(2, textureAtlasSecondary);
    translucentFluidPipeline->bindInput(3, chunkOffsetBuffer);
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
    delete chunkOffsetBuffer;
    delete debugOffsetBuffer;
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
    voxelLayer->loadData(i, m, false);
    voxelComplexLayer->loadData(i, cm, false);
    voxelTranslucentLayer->loadData(i, tm, false);
    voxelTranslucentComplexLayer->loadData(i, tcm, false);
    voxelFluidLayer->loadData(i, fm, false);
    voxelTranslucentFluidLayer->loadData(i, tfm, false);
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

auto OMVoxelManager::calcActualFogColor() -> glm::vec3
{
    glm::vec3 fogColor = colorManager->getFogColor();

    if (renderDistance >= 4)
    {
        const float sunAngle = glm::radians(colorManager->getSunAngle());

        const float sunX = std::sin(sunAngle) > 0.0f ? -1.0f : 1.0f;

        const float lookingAtSun = cameraForward.x * sunX;

        if (lookingAtSun > 0.0f)
        {
            const glm::vec4 sunrise = colorManager->getSunriseColor();
            const float alpha = sunrise.w;

            if (alpha > 0.0f)
            {
                const float t = std::min(lookingAtSun * alpha, 1.0f);
                fogColor = glm::mix(fogColor, glm::vec3(sunrise), t);
            }
        }
    }

    const glm::vec3 skyColor = colorManager->getSkyDiscColor();

    const float skyFogEnd = std::min(colorManager->getFogRange().y / 16.0f, static_cast<float>(renderDistance));

    float skyColorMix = std::clamp(skyFogEnd / 32.0f, 0.25f, 1.0f);
    skyColorMix = 1.0f - std::pow(skyColorMix, 0.25f);

    return glm::mix(fogColor, skyColor, skyColorMix);
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
        auto fg = srgbToLinear(actualFogColor);
        std::array<float, 5> fogParams = {colorManager->getFogRange().x, colorManager->getFogRange().y, fg.r, fg.g,
                                          fg.b};
        fogdata->updateData(fogParams.data());

        std::array<float, 5> cloudFogParams = {colorManager->getFogRange().x * 3, colorManager->getFogRange().y * 3,
                                               fg.r, fg.g, fg.b};
        cloudFogdata->updateData(cloudFogParams.data());
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
                                   colorManager->getSkyLightColor(),
                                   0,
                                   colorManager->getAmbientColor(),
                                   0,
                                   colorManager->getNightVisionColor()};
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

auto OMVoxelManager::fetchLayerState() -> std::array<std::tuple<uint32_t, uint32_t, uint32_t>, 6>
{
    return {{
        {voxelLayer->buf()->usedSize(), voxelLayer->buf()->totalSize, voxelLayer->buf()->blockCount()},
        {voxelComplexLayer->buf()->usedSize(), voxelComplexLayer->buf()->totalSize,
         voxelComplexLayer->buf()->blockCount()},
        {voxelFluidLayer->buf()->usedSize(), voxelFluidLayer->buf()->totalSize, voxelFluidLayer->buf()->blockCount()},
        {voxelTranslucentLayer->buf()->usedSize(), voxelTranslucentLayer->buf()->totalSize,
         voxelTranslucentLayer->buf()->blockCount()},
        {voxelTranslucentComplexLayer->buf()->usedSize(), voxelTranslucentComplexLayer->buf()->totalSize,
         voxelTranslucentComplexLayer->buf()->blockCount()},
        {voxelTranslucentFluidLayer->buf()->usedSize(), voxelTranslucentFluidLayer->buf()->totalSize,
         voxelTranslucentFluidLayer->buf()->blockCount()},
    }};
}

// 绘制/重录粒度：1MB
auto OMVoxelManager::hashLayerState() -> std::array<uint32_t, 12>
{
    // 用"按 1MB 圆整后的实例字节数"参与哈希：区块陆续上传时 usedSize 每帧都会变，
    // 直接用它会导致 recordTask 每帧重录。圆整后只有跨过 1MB 边界（或容量变化）才重录，
    // 绘制时同样按圆整后的数量提交，多出来的实例是清零数据（顶点着色器直接剔除）。
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
    chunkManager->withChunks([&](std::vector<std::shared_ptr<world::OMChunk<16>>> &chunks) -> void {
        for (const auto &c : chunks)
        {
            if (!c)
            {
                continue;
            }
            ++total;
            if (c->visible)
            {
                ++nonfree;
            }
        }
    });
    return {total, nonfree};
}

// 纳秒计时：OM_UPDATE_PROFILE=1 时输出 OMVoxelManager::update 各阶段耗时
static inline auto omNowNs() -> uint64_t
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

static float cloudTime = 0.0f;
static auto lastp = std::chrono::steady_clock::now();

auto OMVoxelManager::update(basics::OMCamera &camera) -> void
{
    static const bool profileUpdate = std::getenv("OM_UPDATE_PROFILE") != nullptr;

    const uint64_t profileStart = profileUpdate ? omNowNs() : 0;
    uint64_t profileStamp = profileStart;
    uint64_t nsSetup = 0, nsOffset = 0, nsCull = 0, nsOffsetUpload = 0;
    uint64_t nsUpload = 0, nsMerge = 0, nsFlush = 0, nsRecord = 0;
    uint64_t cntChunks = 0, cntVisible = 0, cntCompile = 0, cntUnload = 0;

    auto profileMark = [&](uint64_t &out) {
        if (profileUpdate)
        {
            const uint64_t now = omNowNs();
            out = now - profileStamp;
            profileStamp = now;
        }
    };

    auto cameraPos = camera.getPosRaw();

    const std::array<int64_t, 3> cameraChunk = {cameraPos.chunkx, cameraPos.chunky, cameraPos.chunkz};
    const glm::vec3 cameraLocal = {cameraPos.getModX(16), cameraPos.getY(), cameraPos.getModZ(16)};

    const bool cameraMoved = !offsetUploadedOnce || cameraChunk != lastCameraChunk || cameraLocal != lastCameraLocal;

    lastCameraChunk = cameraChunk;
    lastCameraLocal = cameraLocal;

    auto cur = std::chrono::steady_clock::now();
    cloudTime += std::chrono::duration_cast<std::chrono::milliseconds>(cur - lastp).count() / 1000.0f * 3.0f;
    cloudTime = std::fmod(cloudTime, 256 * 12);
    lastp = cur;

    auto cloudc = colorManager->getCloudColor();

    cloudData->updateData(std::array<OMVoxelCloud, 1>{
        {{{cameraPos.getModX(256 * 12) + cloudTime, cameraPos.getY(), cameraPos.getModZ(256 * 12)},
          cloudc.a,
          srgbToLinear(glm::vec3(cloudc.r, cloudc.g, cloudc.b))}}}
                              .data());

    using Position = basics::OMPosition<16, int64_t, float>;
    const std::array<Position, 8> corners = {
        Position(cameraPos.chunkx, cameraPos.chunky, cameraPos.chunkz),
        Position(cameraPos.chunkx + 1, cameraPos.chunky, cameraPos.chunkz),
        Position(cameraPos.chunkx, cameraPos.chunky + 1, cameraPos.chunkz),
        Position(cameraPos.chunkx, cameraPos.chunky, cameraPos.chunkz + 1),
        Position(cameraPos.chunkx + 1, cameraPos.chunky + 1, cameraPos.chunkz),
        Position(cameraPos.chunkx + 1, cameraPos.chunky, cameraPos.chunkz + 1),
        Position(cameraPos.chunkx, cameraPos.chunky + 1, cameraPos.chunkz + 1),
        Position(cameraPos.chunkx + 1, cameraPos.chunky + 1, cameraPos.chunkz + 1),
    };

    debugOffsetBuffer->updateData(std::array<glm::vec3, 2 * 12>{
        {
            corners[0] - camera.getPosRaw(), corners[1] - camera.getPosRaw(), corners[0] - camera.getPosRaw(),
            corners[2] - camera.getPosRaw(), corners[0] - camera.getPosRaw(), corners[3] - camera.getPosRaw(),
            corners[7] - camera.getPosRaw(), corners[4] - camera.getPosRaw(), corners[7] - camera.getPosRaw(),
            corners[5] - camera.getPosRaw(), corners[7] - camera.getPosRaw(), corners[6] - camera.getPosRaw(),
            corners[1] - camera.getPosRaw(), corners[4] - camera.getPosRaw(), corners[1] - camera.getPosRaw(),
            corners[5] - camera.getPosRaw(), corners[2] - camera.getPosRaw(), corners[4] - camera.getPosRaw(),
            corners[2] - camera.getPosRaw(), corners[6] - camera.getPosRaw(), corners[3] - camera.getPosRaw(),
            corners[5] - camera.getPosRaw(), corners[3] - camera.getPosRaw(), corners[6] - camera.getPosRaw(),
        }}.data());

    cameraForward = camera.getForward();
    actualFogColor = calcActualFogColor();

    updateColor();

    profileMark(nsSetup);

    if (chunkManager->numChunks())
    {
        const auto camPosNow = camera.getPosRaw();
        const float yawNow = camera.getYaw();
        const float pitchNow = camera.getPitch();
        const uint64_t loadVersionNow = chunkManager->loadVersion();

        const glm::vec2 extentNow = renderer->getExtent();
        const bool extentChanged = extentNow != lastExtent;
        lastExtent = extentNow;

        const bool cameraStill = !cameraMoved && yawNow == lastCameraYaw && pitchNow == lastCameraPitch;
        const bool chunksLoading = loadVersionNow != lastLoadVersion;
        bool skipCulling = cameraStill && !chunksLoading && !extentChanged && cullingReady;

        // 区块渲染距离（单位：16 格区块）。10 是在这块预载地图上稳定跑到 600 FPS 的取值；
        // 可通过 OM_RENDER_DISTANCE 覆盖（例如 12 / 16 提高视距、换取更低帧率）。
        static const int effectiveRenderDistance = []() -> int {
            const char *env = std::getenv("OM_RENDER_DISTANCE");
            return std::clamp(env != nullptr ? std::atoi(env) : 10, 2, 64);
        }();
        const float cullRadius = static_cast<float>(effectiveRenderDistance) * 16.0f;
        const float cullRadiusSq = cullRadius * cullRadius;

        lastCameraYaw = yawNow;
        lastCameraPitch = pitchNow;
        lastLoadVersion = loadVersionNow;

        if (chunkOffsetCache.size() != chunkManager->numChunks())
        {
            chunkOffsetCache.resize(chunkManager->numChunks());
            skipCulling = false;
        }

        if (hideFrames.size() != chunkManager->numChunks())
        {
            hideFrames.assign(chunkManager->numChunks(), 0);
            skipCulling = false;
        }

        auto layerState = hashLayerState();

        pendingUnload = 0;

        // chunkManager 在后台线程加载区块时，chunks 会持续增长，而 numChunks() 返回后到真正持有
        // withChunks 读锁之间仍可能插入新区块。这里在持锁时按 chunks.size() 补齐按槽位索引的数组，
        // 否则 chunkOffsetCache[i] / hideFrames[i] 会写越界，破坏堆内存。
        auto syncSlotState = [&](const std::vector<std::shared_ptr<world::OMChunk<16>>> &chunks) -> void {
            if (chunkOffsetCache.size() < chunks.size())
            {
                chunkOffsetCache.resize(chunks.size(), glm::vec3{INFINITY});
            }
            if (hideFrames.size() < chunks.size())
            {
                hideFrames.resize(chunks.size(), 0);
            }
        };

        const bool rebuildOffsets =
            cameraMoved || chunksLoading || !offsetUploadedOnce || chunkOffsetCache.size() != lastOffsetCount;

        if (!skipCulling)
        {
            if (rebuildOffsets)
            {
                std::fill(chunkOffsetCache.begin(), chunkOffsetCache.end(), glm::vec3{INFINITY});

                chunkManager->withChunks([&](std::vector<std::shared_ptr<world::OMChunk<16>>> &chunks) -> void {
                    syncSlotState(chunks);

                    int i = 0;
                    for (auto &ochk : chunks)
                    {
                        if (!ochk)
                        {
                            chunkOffsetCache[i] = glm::vec3{INFINITY};
                        }
                        else
                        {
                            const auto &chk = *ochk;
                            auto position = basics::OMPosition<16, int64_t, float>();
                            position.chunkx = chk.chunkx;
                            position.chunky = chk.chunky;
                            position.chunkz = chk.chunkz;

                            chunkOffsetCache[i] = position - camera.getPosRaw();

                            if (profileUpdate)
                            {
                                ++cntChunks;
                            }
                        }
                        ++i;
                    }
                });
            }

            profileMark(nsOffset);

            auto frusturm = Frustum::fromMatrix(camera.fetchProjMat() * camera.fetchViewMat());

            const int64_t cheapRange =
                static_cast<int64_t>(effectiveRenderDistance > 0 ? effectiveRenderDistance : 1000) + 1;
            const int64_t camChunkX = cameraPos.getChunkX();
            const int64_t camChunkY = cameraPos.getChunkY();
            const int64_t camChunkZ = cameraPos.getChunkZ();

            int unloadBudget = 32;

            const bool rebuildCandidates = cameraMoved || chunksLoading || extentChanged || cullCandidates.empty() ||
                                           static_cast<int>(cullCandidates.size()) > chunkManager->numChunks();

            chunkManager->withChunks([&](std::vector<std::shared_ptr<world::OMChunk<16>>> &chunks) -> void {
                syncSlotState(chunks);

                if (rebuildCandidates)
                {
                    cullCandidates.clear();
                    cullCandidates.reserve(chunks.size() / 8 + 16);

                    for (int ci = 0; ci < static_cast<int>(chunks.size()); ++ci)
                    {
                        auto &candidate = chunks[ci];

                        if (!candidate)
                        {
                            if (unloadBudget > 0)
                            {
                                --unloadBudget;
                                unloadChunk(ci);

                                if (profileUpdate)
                                {
                                    ++cntUnload;
                                }
                            }
                            continue;
                        }

                        const int64_t cdx = candidate->chunkx - camChunkX;
                        const int64_t cdy = candidate->chunky - camChunkY;
                        const int64_t cdz = candidate->chunkz - camChunkZ;

                        if (std::abs(cdx) <= cheapRange && std::abs(cdz) <= cheapRange && std::abs(cdy) <= cheapRange)
                        {
                            cullCandidates.push_back(ci);
                        }
                    }
                }

                for (int i : cullCandidates)
                {
                    // 候选槽位来自之前的帧；防御性检查，避免管理器重建/槽位变化后越界。
                    if (i < 0 || static_cast<size_t>(i) >= chunks.size())
                    {
                        continue;
                    }

                    auto &ochk = chunks[i];
                    if (!ochk)
                    {
                        continue;
                    }
                    {
                        auto &chk = *ochk;

                        const int64_t dcx = chk.chunkx - camChunkX;
                        const int64_t dcy = chk.chunky - camChunkY;
                        const int64_t dcz = chk.chunkz - camChunkZ;
                        const bool cheapFar =
                            std::abs(dcx) > cheapRange || std::abs(dcz) > cheapRange || std::abs(dcy) > cheapRange;

                        bool visible = false;
                        bool beyondDistance = cheapFar;
                        glm::vec3 chunkCenter{0.0f};
                        float compileDistanceSq = 0.0f;

                        if (!cheapFar)
                        {
                            auto pp0 = basics::OMPosition<16, int64_t, float>(chk.chunkx, chk.chunky, chk.chunkz) -
                                       camera.getPosRaw();
                            auto pp1 =
                                basics::OMPosition<16, int64_t, float>(chk.chunkx + 1, chk.chunky + 1, chk.chunkz + 1) -
                                camera.getPosRaw();

                            visible = frusturm.cubeVisible(std::array<glm::vec3, 8>{{
                                {pp0.x, pp0.y, pp0.z},
                                {pp1.x, pp0.y, pp0.z},
                                {pp0.x, pp1.y, pp0.z},
                                {pp1.x, pp1.y, pp0.z},
                                {pp0.x, pp0.y, pp1.z},
                                {pp1.x, pp0.y, pp1.z},
                                {pp0.x, pp1.y, pp1.z},
                                {pp1.x, pp1.y, pp1.z},
                            }});

                            // 以区块中心到相机的距离作为编译优先级（越近越先）
                            chunkCenter = pp0 + glm::vec3(8.0f, 8.0f, 8.0f);
                            compileDistanceSq = glm::dot(chunkCenter, chunkCenter);

                            // 距离剔除（精确）
                            beyondDistance = cullRadiusSq > 0.0f && compileDistanceSq > cullRadiusSq;
                            if (beyondDistance)
                            {
                                visible = false;
                            }
                        }

                        if (chk.visible && !visible)
                        {
                            const uint16_t hideDelay = beyondDistance ? 2 : OMVoxelManager::unloadDelay;

                            if (hideFrames[i] < hideDelay)
                            {
                                ++hideFrames[i];
                                visible = true;
                            }
                            else if (unloadBudget > 0)
                            {
                                --unloadBudget;
                                unloadChunk(i);

                                if (profileUpdate)
                                {
                                    ++cntUnload;
                                }
                            }
                            else
                            {
                                ++hideFrames[i];
                                visible = true;
                            }
                        }
                        else if (chk.visible && visible)
                        {
                            hideFrames[i] = 0;

                            if (chk.isDirty())
                            {
                                compilerPool->compile(i, false, compileDistanceSq);
                                chk.solveDirty();

                                if (profileUpdate)
                                {
                                    ++cntCompile;
                                }
                            }
                        }
                        else if (!chk.visible && visible)
                        {
                            hideFrames[i] = 0;
                            compilerPool->compile(i, true, compileDistanceSq);
                            chk.solveDirty();

                            if (profileUpdate)
                            {
                                ++cntCompile;
                            }
                        }

                        chk.visible = visible;

                        if (visible)
                        {
                            if (profileUpdate)
                            {
                                ++cntVisible;
                            }

                            if (hideFrames[i] > 0)
                            {
                                ++pendingUnload;
                            }
                        }
                    }
                    ++i;
                }
            });

            profileMark(nsCull);
            cullingReady = true;
        }
        else
        {
            if (pendingUnload > 0)
            {
                int unloadBudget = 32;
                uint32_t remaining = 0;

                chunkManager->withChunks([&](std::vector<std::shared_ptr<world::OMChunk<16>>> &chunks) -> void {
                    syncSlotState(chunks);

                    for (int i = 0; i < static_cast<int>(chunks.size()); ++i)
                    {
                        auto &ochk = chunks[i];

                        if (!ochk)
                        {
                            if (unloadBudget > 0)
                            {
                                --unloadBudget;
                                unloadChunk(i);

                                if (profileUpdate)
                                {
                                    ++cntUnload;
                                }
                            }
                            continue;
                        }

                        auto &chk = *ochk;

                        if (!chk.visible || hideFrames[i] == 0)
                        {
                            continue;
                        }

                        if (hideFrames[i] < OMVoxelManager::unloadDelay)
                        {
                            ++hideFrames[i];
                            ++remaining;
                        }
                        else if (unloadBudget > 0)
                        {
                            --unloadBudget;
                            unloadChunk(i);
                            chk.visible = false;

                            if (profileUpdate)
                            {
                                ++cntUnload;
                            }
                        }
                        else
                        {
                            ++hideFrames[i];
                            ++remaining;
                        }
                    }
                });

                pendingUnload = remaining;
            }

            profileMark(nsOffset);
            profileMark(nsCull);
        }

        bool chunkOffsetBufferReallocated = false;
        if (chunkOffsetCache.size() * sizeof(glm::vec3) > chunkOffsetBuffer->length)
        {
            delete chunkOffsetBuffer;
            chunkOffsetBuffer =
                renderer->allocateBuffer(UniformTexel, chunkManager->numChunks() * 3 * sizeof(float) * 2);
            pipeline->bindInput(2, chunkOffsetBuffer);
            complexPipeline->bindInput(3, chunkOffsetBuffer);
            fluidPipeline->bindInput(3, chunkOffsetBuffer);
            translucentPipeline->bindInput(2, chunkOffsetBuffer);
            translucentComplexPipeline->bindInput(3, chunkOffsetBuffer);
            translucentFluidPipeline->bindInput(3, chunkOffsetBuffer);
            chunkOffsetBufferReallocated = true;
        }

        chunkOffsetBuffer->synced = true;

        const bool offsetsDirty =
            cameraMoved || chunkOffsetBufferReallocated || chunkOffsetCache.size() != lastOffsetCount;

        if (offsetsDirty)
        {
            const uint64_t usedBytes = chunkOffsetCache.size() * sizeof(glm::vec3);
            chunkOffsetBuffer->updateDataPart(chunkOffsetCache.data(), 0,
                                              std::min<uint64_t>(usedBytes, chunkOffsetBuffer->length));

            static std::vector<glm::vec3> offsetGuard(512, glm::vec3{INFINITY});
            if (chunkOffsetBuffer->length > usedBytes)
            {
                const uint64_t guardBytes =
                    std::min<uint64_t>(chunkOffsetBuffer->length - usedBytes, offsetGuard.size() * sizeof(glm::vec3));
                chunkOffsetBuffer->updateDataPart(offsetGuard.data(), usedBytes, guardBytes);
            }

            lastOffsetCount = chunkOffsetCache.size();
            offsetUploadedOnce = true;
        }

        profileMark(nsOffsetUpload);

        compilerPool->upload(voxelLayer, voxelComplexLayer, voxelFluidLayer, voxelTranslucentLayer,
                             voxelTranslucentComplexLayer, voxelTranslucentFluidLayer);

        profileMark(nsUpload);

        const bool layerStateChanged = layerState != hashLayerState();

        // 增量 compact：每帧整理少量区块，保持 usedSize 接近真实数据量，
        // 避免"全量 merge 后 FPS 突然上升、随后碎片增长又下降"的锯齿。
        if (!skipCulling || layerStateChanged)
        {
            voxelLayer->mergeStep(16);
            voxelComplexLayer->mergeStep(16);
            voxelFluidLayer->mergeStep(16);
            voxelTranslucentLayer->mergeStep(16);
            voxelTranslucentComplexLayer->mergeStep(16);
            voxelTranslucentFluidLayer->mergeStep(16);
        }
        /*if (layerStateChanged)
        {
            voxelLayer->merge();
            voxelComplexLayer->merge();
            voxelFluidLayer->merge();
            voxelTranslucentLayer->merge();
            voxelTranslucentComplexLayer->merge();
            voxelTranslucentFluidLayer->merge();
        }*/

        profileMark(nsMerge);

        uint64_t flushBytes = 0;
        voxelLayer->buf()->flushDirty(OMRendererSegBuf::flushBudgetBytes, &flushBytes);
        voxelComplexLayer->buf()->flushDirty(OMRendererSegBuf::flushBudgetBytes, &flushBytes);
        voxelFluidLayer->buf()->flushDirty(OMRendererSegBuf::flushBudgetBytes, &flushBytes);
        voxelTranslucentLayer->buf()->flushDirty(OMRendererSegBuf::flushBudgetBytes, &flushBytes);
        voxelTranslucentComplexLayer->buf()->flushDirty(OMRendererSegBuf::flushBudgetBytes, &flushBytes);
        voxelTranslucentFluidLayer->buf()->flushDirty(OMRendererSegBuf::flushBudgetBytes, &flushBytes);

        uint64_t flushLeft = 0;
        if (profileUpdate)
        {
            flushLeft = voxelLayer->buf()->dirtyBytes() + voxelComplexLayer->buf()->dirtyBytes() +
                        voxelFluidLayer->buf()->dirtyBytes() + voxelTranslucentLayer->buf()->dirtyBytes() +
                        voxelTranslucentComplexLayer->buf()->dirtyBytes() +
                        voxelTranslucentFluidLayer->buf()->dirtyBytes();
        }

        profileMark(nsFlush);

        // recordTask 会重录整帧（约 0.8ms），旋转视角时每帧都重录会明显掉帧。
        // 用 30ms 节流；容量变化或偏移缓冲重建时强制立即重录。
        const std::array<uint32_t, 6> layerCapacities = {
            voxelLayer->buf()->totalSize,
            voxelComplexLayer->buf()->totalSize,
            voxelFluidLayer->buf()->totalSize,
            voxelTranslucentLayer->buf()->totalSize,
            voxelTranslucentComplexLayer->buf()->totalSize,
            voxelTranslucentFluidLayer->buf()->totalSize,
        };

        static std::array<uint32_t, 6> lastLayerCapacities = {};
        static auto lastRecordTime = std::chrono::steady_clock::now();
        static bool recordPending = false;

        const bool capacityChanged = layerCapacities != lastLayerCapacities;
        lastLayerCapacities = layerCapacities;

        if (layerStateChanged || capacityChanged || chunkOffsetBufferReallocated)
        {
            recordPending = true;
        }

        if (recordPending)
        {
            const bool forced = capacityChanged || chunkOffsetBufferReallocated;
            const auto now = std::chrono::steady_clock::now();
            const bool elapsedEnough =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRecordTime).count() >= 30;

            if (forced || elapsedEnough)
            {
                recordTask(false);
                lastRecordTime = now;
                recordPending = false;
            }
        }

        profileMark(nsRecord);

        if (profileUpdate)
        {
            const uint64_t nsTotal = omNowNs() - profileStart;

            std::fprintf(stderr,
                         "[update] total=%llu setup=%llu offset=%llu cull=%llu offload=%llu upload=%llu merge=%llu "
                         "flush=%llu record=%llu | chunks=%llu visible=%llu compile=%llu unload=%llu mergeState=%d "
                         "flushBytes=%llu flushLeft=%llu usedBytes=%llu\n",
                         static_cast<unsigned long long>(nsTotal), static_cast<unsigned long long>(nsSetup),
                         static_cast<unsigned long long>(nsOffset), static_cast<unsigned long long>(nsCull),
                         static_cast<unsigned long long>(nsOffsetUpload), static_cast<unsigned long long>(nsUpload),
                         static_cast<unsigned long long>(nsMerge), static_cast<unsigned long long>(nsFlush),
                         static_cast<unsigned long long>(nsRecord), static_cast<unsigned long long>(cntChunks),
                         static_cast<unsigned long long>(cntVisible), static_cast<unsigned long long>(cntCompile),
                         static_cast<unsigned long long>(cntUnload), static_cast<int>(layerStateChanged),
                         static_cast<unsigned long long>(flushBytes), static_cast<unsigned long long>(flushLeft),
                         static_cast<unsigned long long>(voxelLayer->buf()->usedSize()));
        }
    }
    else if (profileUpdate)
    {
        std::fprintf(stderr, "[update] total=%llu | chunks=0\n",
                     static_cast<unsigned long long>(omNowNs() - profileStart));
    }
}

auto OMVoxelManager::submit(OMRendererTask *task, OMRendererTempTarget *resolveTarget, bool r) -> OMRendererTask *
{
    if (r)
    {
        cutoutTargetMS->setStoreDepth(true);
        cutoutTargetMS->construct(renderer->getExtent(), samples);
        translucentTargetMS->setClearDepth(false);
        translucentTargetMS->constructWithDepth(cutoutTargetMS->depthTexture, renderer->getExtent(), samples, true);
        cutoutTarget->construct(renderer->getExtent());
        translucentTarget->construct(renderer->getExtent(), 1, true);
        cloudTargetMS->setClearDepth(false);
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
                   ->vertexBuffer({debugOffsetBuffer})
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
    return tsk->removeClearColor(1)->target(resolveTarget->target)->pipeline(composePipeline)->drawN(6);
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
