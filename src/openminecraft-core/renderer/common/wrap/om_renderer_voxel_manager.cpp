#include "glm/fwd.hpp"
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
#include <memory>
#include <vector>

namespace openminecraft::renderer::common::wrap
{
uint64_t cx = 0, cy = 0, cz = 0;
OMVoxelManager::OMVoxelManager(OMRenderer *renderer, OMRendererRenderTarget *resolveTarget, OMRendererTexture *tex,
                               OMRendererTexture *texSec, std::shared_ptr<world::OMChunkManager<16>> man,
                               std::function<void()> rec, OMVoxelHandler *handler,
                               std::function<uint32_t(uint32_t, uint64_t, uint64_t, uint64_t, int, int, int)> converter)
    : logger("OMVoxelManager", this)
{
    this->rec = rec;
    this->renderer = renderer;
    this->chunkManager = man;
    this->voxelHandler = handler;
    this->converter = converter;

    delete compiler.handler;
    compiler.handler = voxelHandler;
    compiler.converter = converter;

    compilerPool = new OMVoxelCompilerPool(*man.get(), compiler);

    cutoutTargetMS = new OMRendererTempTarget(renderer);
    cutoutTargetMS->construct(renderer->getExtent(), samples);

    translucentTargetMS = new OMRendererTempTarget(renderer);
    translucentTargetMS->clearDepth = false;
    translucentTargetMS->construct(renderer->getExtent(), samples, true);

    cutoutTarget = new OMRendererTempTarget(renderer);
    cutoutTarget->construct(renderer->getExtent());
    translucentTarget = new OMRendererTempTarget(renderer);

    basics::OMVertexFormat format, format2, formatComplex, simpleFormat;
    simpleFormat.nextGroup()->decideStruct();
    format.setInstance()
        ->appendPart("voxelPos", basics::Integer)
        ->appendPart("voxelMetadata", basics::Integer)
        ->appendPart("voxelExtra", basics::Integer)
        ->appendPart("voxelExtra2", basics::Integer)
        ->appendPart("voxelExtra3", basics::Integer)
        ->nextGroup()
        ->decideStruct();

    format2.appendPart("voxelPos", basics::Vec3f)->nextGroup()->decideStruct();
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

    pipeline =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("Camera")
            ->input(ImageSampler)
            ->inputName("inTexture")
            ->input(UniformTexelBuffer)
            ->inputName("inChunkPos")
            ->output(cutoutTargetMS->target)
            ->samples(samples)
            ->setCullMode(renderer::common::Back)
            ->setFrontClockwise(true)
            ->shader(renderer->shaderManager.preprocess("core/voxel/voxel.frag.glsl", Fragment, GLSLSource, format))
            ->shader(renderer->shaderManager.preprocess("core/voxel/voxel.vert.glsl", Vertex, GLSLSource, format))
            ->format(format)
            ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
            ->blend(true)
            ->depth(true, true)
            ->depthOp(GreaterOrEqual)
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
                          ->depthOp(GreaterOrEqual)
                          ->buildN();
    translucentPipeline =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("Camera")
            ->input(ImageSampler)
            ->inputName("inTexture")
            ->input(UniformTexelBuffer)
            ->inputName("inChunkPos")
            ->output(translucentTargetMS->target)
            ->samples(samples)
            ->setCullMode(renderer::common::Back)
            ->setFrontClockwise(true)
            ->shader(renderer->shaderManager.preprocess("core/voxel/voxel.oit.frag.glsl", Fragment, GLSLSource, format))
            ->shader(renderer->shaderManager.preprocess("core/voxel/voxel.vert.glsl", Vertex, GLSLSource, format))
            ->format(format)
            ->blendFunc({One, One, Zero, OneMinusSrcAlpha})
            ->blend(true)
            ->depth(true, false)
            ->depthOp(GreaterOrEqual)
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
                                     ->output(translucentTargetMS->target)
                                     ->samples(samples)
                                     ->setCullMode(renderer::common::Back)
                                     ->setFrontClockwise(true)
                                     ->shader(renderer->shaderManager.preprocess(
                                         "core/voxel/voxelcomplex.oit.frag.glsl", Fragment, GLSLSource, formatComplex))
                                     ->shader(renderer->shaderManager.preprocess("core/voxel/voxelcomplex.vert.glsl",
                                                                                 Vertex, GLSLSource, formatComplex))
                                     ->format(formatComplex)
                                     ->blendFunc({One, One, Zero, OneMinusSrcAlpha})
                                     ->blend(true)
                                     ->depth(true, false)
                                     ->depthOp(GreaterOrEqual)
                                     ->buildN();

    debugPipeline =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("Camera")
            ->primitiveType(LineList)
            ->setLineWidth(2.0f)
            ->output(cutoutTargetMS->target)
            ->samples(samples)
            ->shader(
                renderer->shaderManager.preprocess("core/voxel/voxeldebug.frag.glsl", Fragment, GLSLSource, format2))
            ->shader(renderer->shaderManager.preprocess("core/voxel/voxeldebug.vert.glsl", Vertex, GLSLSource, format2))
            ->format(format2)
            ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
            ->blend(true)
            ->depth(true, true)
            ->depthOp(Greater)
            ->buildN();

    skyDiscPipeline = renderer->createPipeline()
                          ->input(UniformBuffer)
                          ->inputName("Camera")
                          ->input(UniformBuffer)
                          ->inputName("SkyDiscData")
                          ->output(cutoutTargetMS->target)
                          ->samples(samples)
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

    composePipeline = renderer->createPipeline()
                          ->input(ImageSampler)
                          ->inputName("inTextureCutout")
                          ->input(ImageSampler)
                          ->inputName("inTextureTranslucent")
                          ->output(resolveTarget)
                          ->samples(1)
                          ->shader(renderer->shaderManager.preprocess("core/voxel/voxelcompose.frag.glsl", Fragment,
                                                                      GLSLSource, simpleFormat))
                          ->shader(renderer->shaderManager.preprocess("core/voxel/voxelcompose.vert.glsl", Vertex,
                                                                      GLSLSource, simpleFormat))
                          ->format(simpleFormat)
                          ->blend(false)
                          ->depth(false, false)
                          ->buildN();

    voxelLayer = new OMVoxelLayer<OMVoxel>(renderer);
    voxelComplexLayer = new OMVoxelLayer<OMVoxelComplex>(renderer);
    voxelTranslucentLayer = new OMVoxelLayer<OMVoxel>(renderer);
    voxelTranslucentComplexLayer = new OMVoxelLayer<OMVoxelComplex>(renderer);

    chunkoffs = renderer->allocateBuffer(UniformTexel, 3 * sizeof(float));
    debugoffs = renderer->allocateBuffer(VertexData, 12 * 2 * 3 * sizeof(float));
    skydisc = renderer->allocateBuffer(Uniform, sizeof(OMVoxelSkyDisc));
    OMVoxelSkyDisc disc = {{0.470, 0.654, 1.0}, 256, {0.198, 0.371, 1.0}, -16};
    skydisc->updateData(&disc);

    textureAtlas = tex;
    textureAtlasSecondary = texSec;

    pipeline->bindInput(1, textureAtlas);
    pipeline->bindInput(2, chunkoffs);
    complexPipeline->bindInput(1, textureAtlas);
    complexPipeline->bindInput(2, textureAtlasSecondary);
    complexPipeline->bindInput(3, chunkoffs);
    translucentPipeline->bindInput(1, textureAtlas);
    translucentPipeline->bindInput(2, chunkoffs);
    translucentComplexPipeline->bindInput(1, textureAtlas);
    translucentComplexPipeline->bindInput(2, textureAtlasSecondary);
    translucentComplexPipeline->bindInput(3, chunkoffs);
    skyDiscPipeline->bindInput(1, skydisc);
}
OMVoxelManager::~OMVoxelManager()
{
    delete compilerPool;
    delete voxelLayer;
    delete voxelComplexLayer;
    delete voxelTranslucentLayer;
    delete voxelTranslucentComplexLayer;
    delete chunkoffs;
    delete debugoffs;
    delete skydisc;
    delete pipeline;
    delete complexPipeline;
    delete translucentPipeline;
    delete translucentComplexPipeline;
    delete debugPipeline;
    delete composePipeline;
    delete translucentTargetMS;
    delete cutoutTargetMS;
    delete cutoutTarget;
    delete translucentTarget;
    delete skyDiscPipeline;
}

void OMVoxelManager::unloadChunk(int i)
{
    std::vector<OMVoxel> m = {}, tm = {};
    std::vector<OMVoxelComplex> cm = {}, tcm = {};
    voxelLayer->loadData(i, m);
    voxelComplexLayer->loadData(i, cm);
    voxelTranslucentLayer->loadData(i, tm);
    voxelTranslucentComplexLayer->loadData(i, tcm);
}

auto OMVoxelManager::update(basics::OMCamera &camera) -> void
{
    auto cc = camera.getPosRaw();
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

    if (chunkManager->numChunks())
    {
        std::vector<glm::vec3> offs = {};
        offs.resize(chunkManager->numChunks());
        auto l = voxelLayer->buf()->totalSize;
        auto l2 = voxelComplexLayer->buf()->totalSize;
        auto l3 = voxelTranslucentLayer->buf()->totalSize;
        auto l4 = voxelTranslucentComplexLayer->buf()->totalSize;
        chunkManager->withChunks([&](std::vector<std::optional<world::OMChunk<16>>> &chunks) -> void {
            int i = 0;
            for (auto &ochk : chunks)
            {
                if (!ochk.has_value())
                {
                    unloadChunk(i);
                    offs[i] = glm::vec3{INFINITY};
                }
                else
                {
                    const auto &chk = ochk.value();
                    auto pp = basics::OMPosition<16, int64_t, float>();
                    pp.chunkx = chk.chunkx;
                    pp.chunky = chk.chunky;
                    pp.chunkz = chk.chunkz;
                    offs[i] = pp - camera.getPosRaw();
                }
                ++i;
            }
        });
        chunkManager->withChunks([&](std::vector<std::optional<world::OMChunk<16>>> &chunks) -> void {
            int i = 0;
            for (auto &ochk : chunks)
            {
                if (ochk.has_value())
                {
                    auto &chk = ochk.value();
                    if (chk.isDirty())
                    {
                        compilerPool->compile(i);
                        chk.solveDirty();
                    }
                }
                ++i;
            }
        });

        if (offs.size() * sizeof(glm::vec3) > chunkoffs->length)
        {
            delete chunkoffs;
            chunkoffs = renderer->allocateBuffer(UniformTexel, chunkManager->numChunks() * 3 * sizeof(float) * 2);
            pipeline->bindInput(2, chunkoffs);
            complexPipeline->bindInput(3, chunkoffs);
            translucentPipeline->bindInput(2, chunkoffs);
            translucentComplexPipeline->bindInput(3, chunkoffs);
        }

        chunkoffs->updateDataPart(offs.data(), 0, offs.size() * sizeof(glm::vec3));

        compilerPool->upload(voxelLayer, voxelComplexLayer, voxelTranslucentLayer, voxelTranslucentComplexLayer);

        if (l != voxelLayer->buf()->totalSize || l2 != voxelComplexLayer->buf()->totalSize ||
            l3 != voxelTranslucentLayer->buf()->totalSize || l4 != voxelTranslucentComplexLayer->buf()->totalSize)
        {
            rec();
        }
    }
}

auto OMVoxelManager::submit(OMRendererTask *task, OMRendererTempTarget *resolveTarget) -> OMRendererTask *
{
    cutoutTargetMS->construct(renderer->getExtent(), samples);
    translucentTargetMS->clearDepth = false;
    translucentTargetMS->constructWithDepth(cutoutTargetMS->depthTexture, renderer->getExtent(), samples, true);
    cutoutTarget->construct(renderer->getExtent());
    translucentTarget->construct(renderer->getExtent(), 1, true);
    composePipeline->bindInput(0, (samples == 1 ? cutoutTargetMS : cutoutTarget)->colorTexture);
    composePipeline->bindInput(1, (samples == 1 ? translucentTargetMS : translucentTarget)->colorTexture);

    auto tsk = task->clearColor({0.198f, 0.371f, 1.0f, 1.0f})
                   ->clearDepth(0.0f)
                   ->target(cutoutTargetMS->target)
                   ->pipeline(skyDiscPipeline)
                   ->drawN(24)
                   ->pipeline(pipeline)
                   ->vertexBuffer({voxelLayer->buf()->buffer})
                   ->drawInstanceN(6, voxelLayer->buf()->totalSize / sizeof(OMVoxel))
                   ->pipeline(complexPipeline)
                   ->vertexBuffer({voxelComplexLayer->buf()->buffer})
                   ->drawInstanceN(6, voxelComplexLayer->buf()->totalSize / sizeof(OMVoxelComplex))
                   ->pipeline(debugPipeline)
                   ->vertexBuffer({debugoffs})
                   ->drawN(2 * 12);

    if (samples != 1)
    {
        tsk->resolve(cutoutTarget->target);
    }

    tsk->clearColor(glm::vec4(0.0, 0.0, 0.0, 1.0))
        ->target(translucentTargetMS->target)
        ->pipeline(translucentPipeline)
        ->vertexBuffer({voxelTranslucentLayer->buf()->buffer})
        ->drawInstanceN(6, voxelTranslucentLayer->buf()->totalSize / sizeof(OMVoxel))
        ->pipeline(translucentComplexPipeline)
        ->vertexBuffer({voxelTranslucentComplexLayer->buf()->buffer})
        ->drawInstanceN(6, voxelTranslucentComplexLayer->buf()->totalSize / sizeof(OMVoxelComplex));

    if (samples != 1)
    {
        tsk->resolve(translucentTarget->target);
    }
    return tsk->target(resolveTarget->target)->pipeline(composePipeline)->drawN(6)->finishN();
}

void OMVoxelManager::bindCameraBuffer(OMRendererBuffer *cameraBuffer)
{
    pipeline->bindInput(0, cameraBuffer);
    debugPipeline->bindInput(0, cameraBuffer);
    complexPipeline->bindInput(0, cameraBuffer);
    translucentPipeline->bindInput(0, cameraBuffer);
    translucentComplexPipeline->bindInput(0, cameraBuffer);
    skyDiscPipeline->bindInput(0, cameraBuffer);
}
} // namespace openminecraft::renderer::common::wrap
