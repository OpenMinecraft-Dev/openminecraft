#include "openminecraft/boot/entrypoint_testrenderer.hpp"

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/geometric.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/event/om_eventbus.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_boxblur.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/world/om_world_chunk.hpp"
#include "openminecraft/world/om_world_chunkmanager.hpp"

#include <chrono>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <utility>

using namespace openminecraft::renderer::common;

namespace openminecraft::boot::test
{
OMTestRenderer::OMTestRenderer(renderer::OMRenderer *renderer, std::function<OMRendererTexture *()> overlay,
                               event::OMEventBusSDL &bus, std::shared_ptr<basics::OMCamera> camera)
    : renderer(renderer), logger("OMTestRenderer", this), OMRendererHandler(renderer), camera(std::move(camera))
{
    this->overlay = overlay;

    format.nextGroup()->decideStruct();

    uniformBuffer = renderer->allocateBuffer(Uniform, sizeof(glm::mat4));
    auto model = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    uniformBuffer->updateData(&model);
    voxelModelBuffer = renderer->allocateBuffer(Uniform, sizeof(glm::mat4));
    glm::mat4 m(1.0f);
    voxelModelBuffer->updateData(&m);
    cameraBuffer = renderer->allocateBuffer(Uniform, sizeof(glm::mat4));
    lightingBuffer = renderer->allocateBuffer(Uniform, sizeof(LightingUniform));

    outputFrg = renderer->shaderManager.preprocess("core/bilt.frag.glsl", Fragment, GLSLSource, format);
    outputVtx = renderer->shaderManager.preprocess("core/bilt.vert.glsl", Vertex, GLSLSource, format);
    auto voxelFrg = renderer->shaderManager.preprocess("core/voxel.frag.glsl", Fragment, GLSLSource, format);
    auto voxelVtx = renderer->shaderManager.preprocess("core/voxel.vert.glsl", Vertex, GLSLSource, format);

    // INFO: core pipeline creation
    mainPipeline = renderer->createPipeline()
                       ->input(ImageSampler)
                       ->inputName("inTexture")
                       ->output(renderer->getDefaultRenderTarget())
                       ->shader(outputFrg)
                       ->shader(outputVtx)
                       ->format(format)
                       ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                       ->blend(true)
                       ->depth(false, false)
                       ->buildN();

    blurHandler = std::make_shared<wrap::OMRendererBoxBlurHandler>(renderer);
    renderer->registerHandler(blurHandler);
    blurHandler->update({8.0f, 32.0f});

    tempTargetMS = new wrap::OMRendererTempTarget(renderer);
    tempTarget = new wrap::OMRendererTempTarget(renderer);
    auto ext = renderer->getExtent();
    tempTargetMS->construct(ext, 4);
    tempTarget->construct(ext);

    textureAtlas = renderer->allocateTexture(16, 16, 8, 4, OMTextureType::Dim2Array, OMTextureArrangement::ColorRgba);
    int i = 0;
    for (auto l : {"dirt", "stone", "cobblestone", "coal_ore", "iron_ore", "dirt", "copper_ore", "diamond_ore"})
    {
        auto imgraw = vfs::fsfetch(fmt::format("/bootassets/external/minecraft/textures/block/{}.png", l));
        specs::png::OMPngFile img2;
        img2.parse(imgraw);
        textureAtlas->updateData(img2.fetchData(), i);
        ++i;
    }
    textureAtlas->mipFilter = Nearest;
    textureAtlas->magFilter = Nearest;
    textureAtlas->minFilter = Nearest;
    textureAtlas->setupSampler();
    auto chunkManager = std::make_shared<world::OMChunkManager<16>>();
    for (int cx = 0; cx < 8; ++cx)
    {
        for (int cy = 0; cy < 8; ++cy)
        {
            for (int cz = 0; cz < 8; ++cz)
            {
                world::OMChunk<16> cnk(cx, cy, cz);
                cnk.setBlock(0, 0, 0, 1);
                cnk.setBlock(0, 1, 0, 1);
                cnk.setBlock(1, 0, 0, 1);
                cnk.setBlock(0, 0, 1, 1);
                chunkManager->loadChunk(cnk);
            }
        }
    }
    voxelManager =
        new wrap::OMVoxelManager(renderer, tempTargetMS->target, textureAtlas, chunkManager, [&]() { record(); });

    voxelManager->pipeline->bindInput(0, voxelModelBuffer);
    voxelManager->pipeline->bindInput(1, cameraBuffer);
    voxelManager->pipeline->bindInput(2, lightingBuffer);
}

static float ang = 0.0f;

void OMTestRenderer::beforeFrame()
{
    if (!timing)
    {
        tp = std::chrono::high_resolution_clock::now();
        timing = true;
    }
    LightingUniform li;
    li.lightDirection = glm::normalize(glm::vec3(0.5, 0.5, 0.5));
    li.lightColor = glm::vec3(1.0);
    li.ambientColor = glm::vec3(0.05);

    auto cam = camera->fetchProjMat() * camera->fetchViewMat();

    cameraBuffer->updateData(&cam);
    lightingBuffer->updateData(&li);
    ang += 0.001f;

    voxelManager->update(*camera);
}

void OMTestRenderer::record()
{
    voxelManager
        ->submit(renderer->fetchTask("scene")
                     ->clearColor({0.198f, 0.371f, 1.0f, 1.0f})
                     ->clearDepth(0.0f)
                     ->target(tempTargetMS->target))
        ->resolve(tempTarget->target)
        ->finishN();
}

void OMTestRenderer::afterFrame()
{
}

void OMTestRenderer::submitTasks()
{
    tempTargetMS->construct(renderer->getExtent(), 4);
    tempTarget->construct(renderer->getExtent());

    mainPipeline->bindInput(0, tempTarget->colorTexture);
    blurHandler->bind(overlay(), tempTarget->colorTexture);

    auto scene = renderer->createTask("scene");
    blurHandler
        ->secondLayerTask(renderer->createTask("main")
                              ->dependOn(scene)
                              ->dependOn(blurHandler->firstLayerTask(scene))
                              ->dependOn(renderer->fetchTask("demiurgeui_compose"))
                              ->target(renderer->getDefaultRenderTarget())
                              ->pipeline(mainPipeline)
                              ->drawN(6))
        ->finishN();

    record();
}
OMTestRenderer::~OMTestRenderer()
{
    delete mainPipeline;
    delete voxelModelBuffer;
    delete uniformBuffer;
    delete cameraBuffer;
    delete lightingBuffer;
    delete voxelManager;
    delete textureAtlas;

    delete tempTargetMS;
    delete tempTarget;
}
} // namespace openminecraft::boot::test
