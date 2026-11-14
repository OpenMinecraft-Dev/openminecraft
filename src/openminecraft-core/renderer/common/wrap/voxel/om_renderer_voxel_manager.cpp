#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_segbuf.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_segbuf.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_manager.hpp"
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

namespace openminecraft::renderer::common::wrap::voxel
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
    OMRenderer *renderer, OMRendererRenderTarget *resolveTarget, const std::string &texTag,
    const std::string &texSecTag, std::shared_ptr<world::OMChunkManager<16>> man, std::function<void(bool)> recordTask,
    OMVoxelHandler *handler,
    std::function<uint32_t(uint32_t, uint64_t, uint64_t, uint64_t, int, int, int)> modelFetcher,
    OMVoxelColorManager *colorman, const std::string &sunTag, const std::string &moonTag, const std::string &cloudTag,
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
    this->sunTex = OMTextureRef(renderer, sunTag);
    this->moonTex = OMTextureRef(renderer, moonTag);
    this->cloudTex = OMTextureRef(renderer, cloudTag);
    this->cloudStats = cloudStats;

    delete compiler.handler;
    compiler.handler = voxelHandler;
    compiler.modelFetcher = modelFetcher;

    compilerPool = new OMVoxelCompilerPool(*man.get(), compiler, 12);

    cutoutTargetMS = new OMRendererTempTarget(renderer, "voxel.cutout.ms");
    cutoutTargetMS->setStoreDepth(true);
    cutoutTargetMS->construct(renderer->getExtent(), samples);

    translucentTargetMS = new OMRendererTempTarget(renderer, "voxel.translucent.ms");
    translucentTargetMS->additionalTexFormats.push_back(R32Sfloat);
    translucentTargetMS->setClearDepth(false);
    translucentTargetMS->construct(renderer->getExtent(), samples, true);

    cloudTargetMS = new OMRendererTempTarget(renderer, "voxel.cloud.ms");
    cloudTargetMS->additionalTexFormats.push_back(R32Sfloat);
    cloudTargetMS->setClearDepth(false);
    cloudTargetMS->setStoreDepth(true);
    cloudTargetMS->construct(renderer->getExtent(), samples);

    cloudTarget = new OMRendererTempTarget(renderer, "voxel.cloud");
    cloudTarget->additionalTexFormats.push_back(R32Sfloat);
    cloudTarget->construct(renderer->getExtent());

    cutoutTarget = new OMRendererTempTarget(renderer, "voxel.cutout");
    cutoutTarget->setClearDepth(false);
    cutoutTarget->construct(renderer->getExtent());
    translucentTarget = new OMRendererTempTarget(renderer, "voxel.translucent");
    translucentTarget->additionalTexFormats.push_back(R32Sfloat);
    lightmap = new OMRendererTempTarget(renderer, "voxel.lightmap");
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
            ->output(cutoutTargetMS->target())
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
                          ->output(cutoutTargetMS->target())
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
                        ->output(cutoutTargetMS->target())
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
            ->output(translucentTargetMS->target())
            ->samples(samples)
            ->setCullMode(renderer::common::None)
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
                                     ->output(translucentTargetMS->target())
                                     ->samples(samples)
                                     ->setCullMode(renderer::common::None)
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
                                   ->output(translucentTargetMS->target())
                                   ->samples(samples)
                                   ->setCullMode(renderer::common::None)
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
                        ->output(resolveTarget)
                        ->samples(1)
                        ->shader(renderer->shaderManager.preprocess("core/voxel/voxeldebug.frag.glsl", Fragment,
                                                                    GLSLSource, voxelDebug))
                        ->shader(renderer->shaderManager.preprocess("core/voxel/voxeldebug.vert.glsl", Vertex,
                                                                    GLSLSource, voxelDebug))
                        ->format(voxelDebug)
                        ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
                        ->blend(true)
                        ->depth(true, false)
                        ->depthOp(Always)
                        ->buildN();

    skyDiscPipeline = renderer->createPipeline()
                          ->input(UniformBuffer)
                          ->inputName("Camera")
                          ->input(UniformBuffer)
                          ->inputName("SkyDiscData")
                          ->output(cutoutTargetMS->target())
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
            ->output(cutoutTargetMS->target())
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
            ->output(cutoutTargetMS->target())
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
            ->output(cutoutTargetMS->target())
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
                          ->output(cutoutTargetMS->target())
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
                        ->output(cloudTargetMS->target())
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
            ->output(translucentTargetMS->target())
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
                           ->output(lightmap->target())
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
            ->output(cutoutTargetMS->target())
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

    voxelLayer = new OMVoxelLayer<OMVoxel>(renderer, "voxel.opaque");
    voxelComplexLayer = new OMVoxelLayer<OMVoxelComplex>(renderer, "voxel.complex");
    voxelTranslucentLayer = new OMVoxelLayer<OMVoxel>(renderer, "voxel.translucent");
    voxelTranslucentComplexLayer = new OMVoxelLayer<OMVoxelComplex>(renderer, "voxel.translucent.complex");
    voxelFluidLayer = new OMVoxelLayer<OMVoxelFluid>(renderer, "voxel.fluid");
    voxelTranslucentFluidLayer = new OMVoxelLayer<OMVoxelFluid>(renderer, "voxel.translucent.fluid");

    chunkOffsetBuffer = OMBufferRef(renderer, "voxel.chunkOffset");
    renderer->registerBuffer(chunkOffsetBuffer.getTag(), UniformTexel, sizeof(glm::vec4));
    chunkOffsetBuffer->format = basics::Vec4f;

    voxelIndexBuffer = OMBufferRef(renderer, "voxel.index");
    renderer->registerBuffer(voxelIndexBuffer.getTag(), VertexIndex, 6 * sizeof(uint32_t));
    {
        const std::array<uint32_t, 6> idx = {0, 1, 2, 2, 3, 0};
        voxelIndexBuffer->updateData(const_cast<uint32_t *>(idx.data()));
    }
    debugOffsetBuffer = OMBufferRef(renderer, "voxel.debugOffset");
    renderer->registerBuffer(debugOffsetBuffer.getTag(), VertexData, 12 * 2 * 3 * sizeof(float));
    skydisc = OMBufferRef(renderer, "voxel.skydisc");
    renderer->registerBuffer(skydisc.getTag(), Uniform, sizeof(OMVoxelSkyDisc));
    fogdata = OMBufferRef(renderer, "voxel.fog");
    renderer->registerBuffer(fogdata.getTag(), Uniform, sizeof(float) * 5);
    cloudFogdata = OMBufferRef(renderer, "voxel.cloud.fog");
    renderer->registerBuffer(cloudFogdata.getTag(), Uniform, sizeof(float) * 5);
    sunrise = OMBufferRef(renderer, "voxel.sunrise");
    renderer->registerBuffer(sunrise.getTag(), Uniform, sizeof(OMVoxelSunrise));
    moonData = OMBufferRef(renderer, "voxel.moon");
    renderer->registerBuffer(moonData.getTag(), Uniform, sizeof(OMVoxelMoon));
    cloudData = OMBufferRef(renderer, "voxel.cloud");
    renderer->registerBuffer(cloudData.getTag(), Uniform, sizeof(OMVoxelCloud));

    {
        auto p = buildVoxelCloud();
        cloudBuffer = OMBufferRef(renderer, "voxel.cloudMesh");
        renderer->registerBuffer(cloudBuffer.getTag(), InstanceData, sizeof(uint32_t) * p.size());
        cloudBuffer->updateData(p.data());
    }

    lightmapData = OMBufferRef(renderer, "voxel.lightmapData");
    renderer->registerBuffer(lightmapData.getTag(), Uniform, sizeof(OMVoxelLightMap));

    textureAtlas = OMTextureRef(renderer, texTag);
    textureAtlasSecondary = OMTextureRef(renderer, texSecTag);

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
    starBuffer = OMBufferRef(renderer, "voxel.star");
    renderer->registerBuffer(starBuffer.getTag(), InstanceData, starData.size() * sizeof(float));
    starBuffer->updateData(starData.data());

    starBaseData = OMBufferRef(renderer, "voxel.starBase");
    renderer->registerBuffer(starBaseData.getTag(), Uniform, sizeof(float) * 2);

    pipeline->bindInput(1, textureAtlas);
    pipeline->bindInput(2, chunkOffsetBuffer);
    pipeline->bindInput(3, fogdata);
    pipeline->bindInput(4, lightmap->colorTexture());

    complexPipeline->bindInput(1, textureAtlas);
    complexPipeline->bindInput(2, textureAtlasSecondary);
    complexPipeline->bindInput(3, chunkOffsetBuffer);
    complexPipeline->bindInput(4, fogdata);
    complexPipeline->bindInput(5, lightmap->colorTexture());

    fluidPipeline->bindInput(1, textureAtlas);
    fluidPipeline->bindInput(2, textureAtlasSecondary);
    fluidPipeline->bindInput(3, chunkOffsetBuffer);
    fluidPipeline->bindInput(4, fogdata);
    fluidPipeline->bindInput(5, lightmap->colorTexture());

    translucentPipeline->bindInput(1, textureAtlas);
    translucentPipeline->bindInput(2, chunkOffsetBuffer);
    translucentPipeline->bindInput(3, fogdata);
    translucentPipeline->bindInput(4, lightmap->colorTexture());

    translucentComplexPipeline->bindInput(1, textureAtlas);
    translucentComplexPipeline->bindInput(2, textureAtlasSecondary);
    translucentComplexPipeline->bindInput(3, chunkOffsetBuffer);
    translucentComplexPipeline->bindInput(4, fogdata);
    translucentComplexPipeline->bindInput(5, lightmap->colorTexture());

    translucentFluidPipeline->bindInput(1, textureAtlas);
    translucentFluidPipeline->bindInput(2, textureAtlasSecondary);
    translucentFluidPipeline->bindInput(3, chunkOffsetBuffer);
    translucentFluidPipeline->bindInput(4, fogdata);
    translucentFluidPipeline->bindInput(5, lightmap->colorTexture());

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
    delete compilerPool;
    delete voxelLayer;
    delete voxelComplexLayer;
    delete voxelTranslucentLayer;
    delete voxelTranslucentComplexLayer;
    delete voxelFluidLayer;
    delete voxelTranslucentFluidLayer;
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
    delete starPipeline;
    delete cloudPipeline;
    delete cloudComposePipeline;
}

void OMVoxelManager::unloadChunk(int i)
{
    if (static_cast<size_t>(i) < sectionVisibility.size())
    {
        sectionVisibility[i] = 0;
        ++graphVersion;
    }

    if (static_cast<size_t>(i) < chunkOffsetCache.size())
    {
        if (chunkOffsetCache[i] != glm::vec4{INFINITY})
        {
            chunkOffsetCache[i] = glm::vec4{INFINITY};
            offsetCacheDirty = true;
        }
    }

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

// Draw/re-record granularity: 1MB
auto OMVoxelManager::hashLayerState() -> std::array<uint32_t, 12>
{
    // Hash the instance byte count rounded up to 1MB: usedSize changes every frame while chunks upload,
    // so using it directly would re-record every frame. After rounding, only crossing a 1MB boundary (or a capacity
    // change) re-records, and draws submit the rounded count too; extra instances are zeroed data rejected by the
    // vertex shader.
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

static float cloudTime = 0.0f;
static auto lastp = std::chrono::steady_clock::now();

auto OMVoxelManager::update(basics::OMCamera &camera) -> void
{
    // BFS occlusion culling measured as a net loss (6~7ms main-thread spikes during rotation).
    constexpr bool bfsOcclusionEnabled = false;

    auto cameraPos = camera.getPosRaw();

    const std::array<int64_t, 3> cameraChunk = {cameraPos.chunkx, cameraPos.chunky, cameraPos.chunkz};
    const glm::vec3 cameraLocal = {cameraPos.getModX(16), cameraPos.getY(), cameraPos.getModZ(16)};

    const bool cameraMoved = !offsetUploadedOnce || cameraChunk != lastCameraChunk || cameraLocal != lastCameraLocal;

    lastCameraChunk = cameraChunk;
    lastCameraLocal = cameraLocal;

    // Clouds are pinned at camera height (near the horizon): the layer is invisible when the view's top edge is below
    // the horizon
    renderClouds = (camera.getPitch() + camera.getFov() * 0.5f) > -2.0f;

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

        // Chunk render distance (in 16-block chunks). 10 gave a stable 600 FPS on this preloaded map;
        // override with OM_RENDER_DISTANCE (e.g. 12 / 16 for more view distance at lower FPS).
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
            chunkOffsetCache.resize(chunkManager->numChunks(), glm::vec4{INFINITY});
            skipCulling = false;
        }

        if (hideFrames.size() != chunkManager->numChunks())
        {
            hideFrames.assign(chunkManager->numChunks(), 0);
            skipCulling = false;
        }

        auto layerState = hashLayerState();

        pendingUnload = 0;

        // Occlusion culling: BFS from the camera's section, using the visibility matrix to tell whether a path can pass
        // through a section. Recompute only when the camera changes section or visibility data changes; data-driven
        // recomputes are throttled to 100ms.
        if (sectionVisibility.size() < static_cast<size_t>(chunkManager->numChunks()))
        {
            sectionVisibility.resize(static_cast<size_t>(chunkManager->numChunks()), 0);
        }

        const std::array<int64_t, 3> cameraSection = {cameraPos.chunkx, cameraPos.chunky, cameraPos.chunkz};
        const bool bfsCameraChanged = cameraSection != lastBfsSection;
        const bool bfsGraphChanged = graphVersion != bfsGraphVersion;
        const bool bfsThrottled = !bfsCameraChanged && std::chrono::duration_cast<std::chrono::milliseconds>(
                                                           std::chrono::steady_clock::now() - lastBfsTime)
                                                               .count() < 100;

        // Incrementally fill visibility data for candidate sections in range (limited per frame to avoid starving
        // geometry compilation)
        if (bfsOcclusionEnabled && !cullCandidates.empty())
        {
            int requested = 0;

            for (size_t n = 0; n < cullCandidates.size() && requested < 48; ++n)
            {
                const size_t idx = (visFillCursor + n) % cullCandidates.size();
                const int slot = cullCandidates[idx];

                if (static_cast<size_t>(slot) < sectionVisibility.size() && (sectionVisibility[slot] >> 63) != 0)
                {
                    continue;
                }

                compilerPool->requestVisibility(slot);
                ++requested;
            }

            visFillCursor = (visFillCursor + 48) % cullCandidates.size();
        }

        if (bfsOcclusionEnabled && (bfsCameraChanged || bfsGraphChanged) && !bfsThrottled)
        {
            lastBfsSection = cameraSection;
            lastBfsTime = std::chrono::steady_clock::now();
            bfsGraphVersion = graphVersion;
            bfsValid = false;

            chunkManager->withChunkMap([&](auto &chunkMap, const auto &chunks) -> void {
                bfsVisibleMask.assign(chunks.size(), 0);

                std::vector<uint8_t> entryMask(chunks.size(), 0);
                std::vector<int> bfsQueue;
                bfsQueue.reserve(4096);

                auto camIt = chunkMap.find(world::OMChunkIndex{cameraSection[0], cameraSection[1], cameraSection[2]});
                if (camIt == chunkMap.end())
                {
                    return;
                }

                const int64_t boxRange =
                    static_cast<int64_t>(effectiveRenderDistance > 0 ? effectiveRenderDistance : 1000) + 1;
                const int camSlot = camIt->second;
                entryMask[camSlot] = 0x3F;
                bfsQueue.push_back(camSlot);
                bfsVisibleMask[camSlot] = 1;

                static constexpr int dirs[6][3] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
                static constexpr int opposite[6] = {1, 0, 3, 2, 5, 4};

                for (size_t qi = 0; qi < bfsQueue.size(); ++qi)
                {
                    const int slot = bfsQueue[qi];
                    const auto &slotChunk = chunks[slot];

                    if (!slotChunk)
                    {
                        continue;
                    }

                    const uint8_t em = entryMask[slot];
                    const uint64_t rawVis = static_cast<size_t>(slot) < sectionVisibility.size()
                                                ? sectionVisibility[slot]
                                                : static_cast<uint64_t>(0);
                    const bool visKnown = (rawVis >> 63) != 0;
                    const uint64_t visMatrix = rawVis & 0x7FFFFFFFFFFFFFFFull;

                    uint8_t exits = 0;

                    if (!visKnown)
                    {
                        // Not compiled yet: treat as fully permeable (conservative, avoids wrong culling)
                        exits = 0x3F;
                    }
                    else
                    {
                        for (int a = 0; a < 6; ++a)
                        {
                            if ((em & (1 << a)) != 0)
                            {
                                exits |= static_cast<uint8_t>((visMatrix >> (a * 6)) & 0x3F);
                            }
                        }
                    }

                    if (exits == 0)
                    {
                        continue;
                    }

                    for (int d = 0; d < 6; ++d)
                    {
                        if ((exits & (1 << d)) == 0)
                        {
                            continue;
                        }

                        const int64_t nx = slotChunk->chunkx + dirs[d][0];
                        const int64_t ny = slotChunk->chunky + dirs[d][1];
                        const int64_t nz = slotChunk->chunkz + dirs[d][2];

                        if (std::llabs(nx - cameraSection[0]) > boxRange ||
                            std::llabs(ny - cameraSection[1]) > boxRange ||
                            std::llabs(nz - cameraSection[2]) > boxRange)
                        {
                            continue;
                        }

                        auto nIt = chunkMap.find(world::OMChunkIndex{nx, ny, nz});
                        if (nIt == chunkMap.end())
                        {
                            continue;
                        }

                        const int nSlot = nIt->second;
                        if (!chunks[nSlot])
                        {
                            continue;
                        }

                        const uint8_t newEntry = static_cast<uint8_t>(1 << opposite[d]);
                        if ((entryMask[nSlot] & newEntry) == 0)
                        {
                            entryMask[nSlot] |= newEntry;
                            bfsVisibleMask[nSlot] = 1;
                            bfsQueue.push_back(nSlot);
                        }
                    }
                }

                bfsValid = true;
            });
        }

        auto syncSlotState = [&](const std::vector<std::shared_ptr<world::OMChunk<16>>> &chunks) -> void {
            if (chunkOffsetCache.size() < chunks.size())
            {
                chunkOffsetCache.resize(chunks.size(), glm::vec4{INFINITY});
            }
            if (hideFrames.size() < chunks.size())
            {
                hideFrames.resize(chunks.size(), 0);
            }
        };

        auto setOffset = [&](int index, const glm::vec4 &value) {
            if (index < 0 || static_cast<size_t>(index) >= chunkOffsetCache.size())
            {
                return;
            }

            if (chunkOffsetCache[index] != value)
            {
                chunkOffsetCache[index] = value;
                offsetCacheDirty = true;
            }
        };

        if (!skipCulling)
        {
            auto frusturm = Frustum::fromMatrix(camera.fetchProjMat() * camera.fetchViewMat());

            const int64_t cheapRange =
                static_cast<int64_t>(effectiveRenderDistance > 0 ? effectiveRenderDistance : 1000) + 1;
            const int64_t camChunkX = cameraPos.getChunkX();
            const int64_t camChunkY = cameraPos.getChunkY();
            const int64_t camChunkZ = cameraPos.getChunkZ();

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
                            if (static_cast<size_t>(ci) < chunkOffsetCache.size() &&
                                !std::isinf(chunkOffsetCache[ci].x))
                            {
                                unloadChunk(ci);
                            }
                            setOffset(ci, glm::vec4{INFINITY});
                            continue;
                        }

                        const int64_t cdx = candidate->chunkx - camChunkX;
                        const int64_t cdy = candidate->chunky - camChunkY;
                        const int64_t cdz = candidate->chunkz - camChunkZ;

                        if (std::abs(cdx) <= cheapRange && std::abs(cdz) <= cheapRange && std::abs(cdy) <= cheapRange)
                        {
                            cullCandidates.push_back(ci);
                        }
                        else
                        {
                            if (candidate->visible)
                            {
                                unloadChunk(ci);
                            }
                            candidate->visible = false;
                            setOffset(ci, glm::vec4{INFINITY});
                        }
                    }
                }

                for (int i : cullCandidates)
                {
                    if (i < 0 || static_cast<size_t>(i) >= chunks.size())
                    {
                        continue;
                    }

                    auto &ochk = chunks[i];
                    if (!ochk)
                    {
                        continue;
                    }

                    auto &chk = *ochk;
                    setOffset(i, glm::vec4{INFINITY});

                    if (chk.isEmpty())
                    {
                        if (chk.visible)
                        {
                            unloadChunk(i);
                            chk.visible = false;
                        }
                        hideFrames[i] = 0;
                        continue;
                    }

                    const int64_t dcx = chk.chunkx - camChunkX;
                    const int64_t dcy = chk.chunky - camChunkY;
                    const int64_t dcz = chk.chunkz - camChunkZ;
                    const bool cheapFar =
                        std::abs(dcx) > cheapRange || std::abs(dcz) > cheapRange || std::abs(dcy) > cheapRange;

                    bool visible = false;
                    bool beyondDistance = cheapFar;
                    glm::vec3 chunkCenter{0.0f};
                    float compileDistanceSq = 0.0f;
                    auto chunkOffset =
                        basics::OMPosition<16, int64_t, float>(chk.chunkx, chk.chunky, chk.chunkz) - camera.getPosRaw();

                    if (!cheapFar)
                    {
                        auto pp0 = chunkOffset;
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

                        chunkCenter = pp0 + glm::vec3(8.0f, 8.0f, 8.0f);
                        compileDistanceSq = glm::dot(chunkCenter, chunkCenter);

                        beyondDistance = cullRadiusSq > 0.0f && compileDistanceSq > cullRadiusSq;
                        if (beyondDistance)
                        {
                            visible = false;
                        }
                    }

                    if (bfsOcclusionEnabled && bfsValid && static_cast<size_t>(i) < bfsVisibleMask.size() &&
                        bfsVisibleMask[i] == 0)
                    {
                        visible = false;
                    }

                    if (!visible)
                    {
                        if (chk.visible)
                        {
                            unloadChunk(i);
                            chk.visible = false;
                        }
                        hideFrames[i] = 0;
                        continue;
                    }

                    hideFrames[i] = 0;
                    setOffset(i, glm::vec4(chunkOffset, 1.0f));

                    if (!chk.visible)
                    {
                        compilerPool->compile(i, true, compileDistanceSq);
                    }
                    else if (chk.isDirty())
                    {
                        compilerPool->compile(i, false, compileDistanceSq);
                    }

                    chk.solveDirty();
                    chk.visible = true;
                }
            });

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
        }

        bool chunkOffsetBufferReallocated = false;
        if (chunkOffsetCache.size() * sizeof(glm::vec4) > chunkOffsetBuffer->length)
        {
            renderer->unregisterBuffer(chunkOffsetBuffer.getTag());
            static uint64_t chunkOffsetGeneration = 1;
            chunkOffsetBuffer = OMBufferRef(renderer, "voxel.chunkOffset.g" + std::to_string(chunkOffsetGeneration++));
            renderer->registerBuffer(chunkOffsetBuffer.getTag(), UniformTexel,
                                     static_cast<uint64_t>(chunkManager->numChunks()) * sizeof(glm::vec4) * 2);
            chunkOffsetBuffer->format = basics::Vec4f;
            pipeline->bindInput(2, chunkOffsetBuffer);
            complexPipeline->bindInput(3, chunkOffsetBuffer);
            fluidPipeline->bindInput(3, chunkOffsetBuffer);
            translucentPipeline->bindInput(2, chunkOffsetBuffer);
            translucentComplexPipeline->bindInput(3, chunkOffsetBuffer);
            translucentFluidPipeline->bindInput(3, chunkOffsetBuffer);
            chunkOffsetBufferReallocated = true;
        }

        chunkOffsetBuffer->synced = true;

        const bool offsetsDirty = cameraMoved || chunkOffsetBufferReallocated || offsetCacheDirty ||
                                  chunkOffsetCache.size() != lastOffsetCount;

        if (offsetsDirty)
        {
            const uint64_t usedBytes = chunkOffsetCache.size() * sizeof(glm::vec4);
            chunkOffsetBuffer->updateDataPart(chunkOffsetCache.data(), 0,
                                              std::min<uint64_t>(usedBytes, chunkOffsetBuffer->length));

            static std::vector<glm::vec4> offsetGuard(512, glm::vec4{INFINITY});
            if (chunkOffsetBuffer->length > usedBytes)
            {
                const uint64_t guardBytes =
                    std::min<uint64_t>(chunkOffsetBuffer->length - usedBytes, offsetGuard.size() * sizeof(glm::vec4));
                chunkOffsetBuffer->updateDataPart(offsetGuard.data(), usedBytes, guardBytes);
            }

            lastOffsetCount = chunkOffsetCache.size();
            offsetUploadedOnce = true;
            offsetCacheDirty = false;
        }

        compilerPool->upload(
            voxelLayer, voxelComplexLayer, voxelFluidLayer, voxelTranslucentLayer, voxelTranslucentComplexLayer,
            voxelTranslucentFluidLayer, [&](int slot, world::OMChunkIndex origin, uint64_t vis) {
                auto chk = chunkManager->getChunk(slot);
                if (!chk || chk->chunkx != origin.x || chk->chunky != origin.y || chk->chunkz != origin.z)
                {
                    return;
                }

                if (sectionVisibility.size() <= static_cast<size_t>(slot))
                {
                    sectionVisibility.resize(static_cast<size_t>(slot) + 1, 0);
                }

                sectionVisibility[slot] = vis;
                ++graphVersion;
            });

        const bool layerStateChanged = layerState != hashLayerState();

        {
            const auto compactStart = std::chrono::steady_clock::now();

            auto fragmented = [](auto *layer) -> bool {
                const uint64_t live = layer->liveBytesCount();
                return layer->buf()->usedSize() > live + live / 4 + 128 * 1024;
            };

            auto budgetLeft = [&]() -> bool {
                return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - compactStart)
                           .count() < 1.0;
            };

            if (fragmented(voxelLayer) && budgetLeft())
            {
                voxelLayer->mergeStepTimed(0.3);
            }
            if (fragmented(voxelComplexLayer) && budgetLeft())
            {
                voxelComplexLayer->mergeStepTimed(0.3);
            }
            if (fragmented(voxelFluidLayer) && budgetLeft())
            {
                voxelFluidLayer->mergeStepTimed(0.3);
            }
            if (fragmented(voxelTranslucentLayer) && budgetLeft())
            {
                voxelTranslucentLayer->mergeStepTimed(0.3);
            }
            if (fragmented(voxelTranslucentComplexLayer) && budgetLeft())
            {
                voxelTranslucentComplexLayer->mergeStepTimed(0.3);
            }
            if (fragmented(voxelTranslucentFluidLayer) && budgetLeft())
            {
                voxelTranslucentFluidLayer->mergeStepTimed(0.3);
            }
        }

        voxelLayer->buf()->flushDirty();
        voxelComplexLayer->buf()->flushDirty();
        voxelFluidLayer->buf()->flushDirty();
        voxelTranslucentLayer->buf()->flushDirty();
        voxelTranslucentComplexLayer->buf()->flushDirty();
        voxelTranslucentFluidLayer->buf()->flushDirty();

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
    }
}

auto OMVoxelManager::submit(OMRendererTask *task, OMRendererTempTarget *resolveTarget, bool r) -> OMRendererTask *
{
    if (r)
    {
        cutoutTargetMS->setStoreDepth(true);
        cutoutTargetMS->construct(renderer->getExtent(), samples);
        translucentTargetMS->setClearDepth(false);
        translucentTargetMS->constructWithDepth(cutoutTargetMS->depthTexTag(), renderer->getExtent(), samples, true);
        cutoutTarget->construct(renderer->getExtent());
        translucentTarget->construct(renderer->getExtent(), 1, true);
        cloudTargetMS->setClearDepth(false);
        cloudTargetMS->constructWithDepth(cutoutTargetMS->depthTexTag(), renderer->getExtent(), samples);
        cloudTarget->construct(renderer->getExtent());
        composePipeline->bindInput(0, (samples == 1 ? cutoutTargetMS : cutoutTarget)->colorTexture());
        composePipeline->bindInput(1, (samples == 1 ? translucentTargetMS : translucentTarget)->colorTexture());
        composePipeline->bindInput(2, (samples == 1 ? translucentTargetMS : translucentTarget)->additionalTex(0));
        cloudComposePipeline->bindInput(0, (samples == 1 ? cloudTargetMS : cloudTarget)->colorTexture());
        cloudComposePipeline->bindInput(1, (samples == 1 ? cloudTargetMS : cloudTarget)->additionalTex(0));
    }

    auto tsk = task->target(lightmap->target())
                   ->beginDebugTag("environment")
                   ->pipeline(lightmapPipeline)
                   ->drawN(6)
                   ->clearColor({0.0f, 0.0f, 0.0f, 0.0f})
                   ->clearDepth(1.0f)
                   ->target(cutoutTargetMS->target())
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
                   ->vertexBuffer({voxelLayer->buf()->buffer()})
                   ->indexBuffer(voxelIndexBuffer)
                   ->drawIndexedInstanceN(6, voxelLayer->buf()->usedSize() / sizeof(OMVoxel))
                   ->pipeline(complexPipeline)
                   ->vertexBuffer({voxelComplexLayer->buf()->buffer()})
                   ->indexBuffer(voxelIndexBuffer)
                   ->drawIndexedInstanceN(6, voxelComplexLayer->buf()->usedSize() / sizeof(OMVoxelComplex))
                   ->pipeline(fluidPipeline)
                   ->vertexBuffer({voxelFluidLayer->buf()->buffer()})
                   ->indexBuffer(voxelIndexBuffer)
                   ->drawIndexedInstanceN(6, voxelFluidLayer->buf()->usedSize() / sizeof(OMVoxelFluid))
                   ->endDebugTag();

    if (samples != 1)
    {
        tsk->resolve(cutoutTarget->target());
    }

    if (renderClouds)
    {
        tsk->beginDebugTag("clouds")
            ->clearColor(glm::vec4(0.0), 0)
            ->clearColor(glm::vec4(1.0), 1)
            ->target(cloudTargetMS->target())
            ->pipeline(cloudPipeline)
            ->vertexBuffer({cloudBuffer})
            ->drawInstanceN(36, cloudBuffer->length / sizeof(uint32_t))
            ->endDebugTag();
    }
    else
    {
        // Whole layer invisible: only clear the target so later compose does not sample last frame's clouds
        tsk->beginDebugTag("clouds(skip)")
            ->clearColor(glm::vec4(0.0), 0)
            ->clearColor(glm::vec4(1.0), 1)
            ->target(cloudTargetMS->target())
            ->endDebugTag();
    }

    if (samples != 1)
    {
        tsk->resolve(cloudTarget->target());
    }

    tsk->beginDebugTag("translucent chunks")
        ->clearColor(glm::vec4(0.0), 0)
        ->clearColor(glm::vec4(1.0), 1)
        ->target(translucentTargetMS->target())
        ->pipeline(cloudComposePipeline)
        ->drawN(6)
        ->pipeline(translucentPipeline)
        ->vertexBuffer({voxelTranslucentLayer->buf()->buffer()})
        ->indexBuffer(voxelIndexBuffer)
        ->drawIndexedInstanceN(6, voxelTranslucentLayer->buf()->usedSize() / sizeof(OMVoxel))
        ->pipeline(translucentComplexPipeline)
        ->vertexBuffer({voxelTranslucentComplexLayer->buf()->buffer()})
        ->indexBuffer(voxelIndexBuffer)
        ->drawIndexedInstanceN(6, voxelTranslucentComplexLayer->buf()->usedSize() / sizeof(OMVoxelComplex))
        ->pipeline(translucentFluidPipeline)
        ->vertexBuffer({voxelTranslucentFluidLayer->buf()->buffer()})
        ->indexBuffer(voxelIndexBuffer)
        ->drawIndexedInstanceN(6, voxelTranslucentFluidLayer->buf()->usedSize() / sizeof(OMVoxelFluid))
        ->endDebugTag();

    if (samples != 1)
    {
        tsk->resolve(translucentTarget->target());
    }
    return tsk->removeClearColor(1)
        ->target(resolveTarget->target())
        ->pipeline(composePipeline)
        ->drawN(6)
        ->pipeline(debugPipeline)
        ->vertexBuffer({debugOffsetBuffer})
        ->drawN(2 * 12);
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
} // namespace openminecraft::renderer::common::wrap::voxel
