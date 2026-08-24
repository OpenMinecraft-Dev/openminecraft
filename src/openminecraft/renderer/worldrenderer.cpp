#include "openminecraft-shell/renderer/worldrenderer.hpp"

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/geometric.hpp"
#include "openminecraft-shell/data/block/om_block_registery.hpp"
#include "openminecraft-shell/data/block/om_blockstate_resolver.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft-shell/data/om_model_precompiler.hpp"
#include "openminecraft-shell/data/om_textureatlas.hpp"
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
#include "openminecraft/world/om_world_chunk.hpp"
#include "openminecraft/world/om_world_chunkmanager.hpp"

#include <chrono>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <utility>

using namespace openminecraft::renderer::common;

namespace openminecraftshell::renderer
{
OMWorldRenderer::OMWorldRenderer(OMRenderer *renderer, std::function<OMRendererTexture *()> overlay,
                                 event::OMEventBusSDL &bus, std::shared_ptr<basics::OMCamera> camera)
    : OMRendererHandler(renderer), camera(std::move(camera)), logger("OMWorldRenderer", this), renderer(renderer)
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

    textureAtlas = new data::OMTextureAtlas("/external", renderer);

    voxelHandler = new data::OMModelPrecompiler("/external", textureAtlas);
    blockstateResolver = new data::block::OMBlockstateResolver("/external", *voxelHandler);
    for (auto &p : data::block::blockRegistery.nameToId)
    {
        blockstateResolver->resolve(p.first);
    }
    blockstateResolver->fetchModel(data::OMIdentifier("minecraft:air"), "");
    auto blockStone = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:stone"), "");
    auto blockCobblestone = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:cobblestone"), "");
    auto blockCoalOre = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:coal_ore"), "");
    auto blockIronOre = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:iron_ore"), "");
    auto blockDirt = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:dirt"), "");
    auto blockCopperOre = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:copper_ore"), "");
    auto blockTallGrassL = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:tall_grass"), "half=lower");
    auto blockTallGrassH = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:tall_grass"), "half=upper");
    auto blockCherryStairs = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:cherry_stairs"),
                                                            "facing=east,half=bottom,shape=straight");
    auto blockCherryDoorL = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:cherry_door"),
                                                           "facing=east,half=lower,hinge=left,open=false");
    auto blockCherryDoorH = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:cherry_door"),
                                                           "facing=east,half=upper,hinge=left,open=false");
    auto blockCherrySign = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:cherry_hanging_sign"),
                                                          "attached=false,rotation=3");
    auto blockCherryButton = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:cherry_button"),
                                                            "face=floor,facing=north,powered=false");
    auto blockCherryShelf =
        blockstateResolver->fetchModel(data::OMIdentifier("minecraft:cherry_shelf"), "facing=east,powered=false");
    auto blockGrassBlock = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:grass_block"), "snowy=false");
    auto blockCherryFence = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:cherry_fence_gate"),
                                                           "facing=south,in_wall=false,open=true");
    auto blockRail = blockstateResolver->fetchModel(data::OMIdentifier("minecraft:rail"), "shape=north_south");
    textureAtlas->build();

    auto chunkManager = std::make_shared<world::OMChunkManager<16>>();
    for (int cx = -1; cx < 8; ++cx)
    {
        for (int cy = 0; cy < 8; ++cy)
        {
            for (int cz = -1; cz < 8; ++cz)
            {
                world::OMChunk<16> cnk(cx + 0x7fffffffffffffff, cy, cz);
                cnk.setBlock(0, 0, 0, blockStone);
                cnk.setBlock(0, 1, 1, blockCopperOre);
                cnk.setBlock(2, 0, 0, blockGrassBlock);
                cnk.setBlock(2, 1, 0, blockTallGrassL);
                cnk.setBlock(2, 2, 0, blockTallGrassH);
                cnk.setBlock(1, 1, 0, blockCherryStairs);
                cnk.setBlock(3, 1, 0, blockCherryDoorL);
                cnk.setBlock(3, 2, 0, blockCherryDoorH);
                cnk.setBlock(0, 1, 0, blockCherryShelf);
                cnk.setBlock(0, 2, 1, blockCherrySign);
                cnk.setBlock(0, 1, 2, blockCopperOre);
                cnk.setBlock(15, 1, 1, blockCopperOre);
                cnk.setBlock(15, 0, 0, blockCobblestone);
                cnk.setBlock(15, 1, 2, blockCopperOre);
                cnk.setBlock(15, 1, 0, blockCherryButton);
                cnk.setBlock(15, 2, 2, blockCherryFence);
                cnk.setBlock(15, 2, 1, blockRail);
                chunkManager->loadChunk(cnk);
            }
        }
    }

    voxelManager = new wrap::OMVoxelManager(
        renderer, tempTargetMS->target, textureAtlas->texture, textureAtlas->textureSecondary, chunkManager,
        [&]() { record(); }, this->voxelHandler);

    voxelManager->pipeline->bindInput(0, cameraBuffer);
    voxelManager->debugPipeline->bindInput(0, cameraBuffer);
    voxelManager->complexPipeline->bindInput(0, cameraBuffer);
}

static float ang = 0.0f;

void OMWorldRenderer::beforeFrame()
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

void OMWorldRenderer::record()
{
    voxelManager
        ->submit(renderer->fetchTask("scene")
                     ->clearColor({0.198f, 0.371f, 1.0f, 1.0f})
                     ->clearDepth(0.0f)
                     ->target(tempTargetMS->target))
        ->resolve(tempTarget->target)
        ->finishN();
}

void OMWorldRenderer::afterFrame()
{
}

void OMWorldRenderer::submitTasks()
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
OMWorldRenderer::~OMWorldRenderer()
{
    delete mainPipeline;
    delete voxelModelBuffer;
    delete uniformBuffer;
    delete cameraBuffer;
    delete lightingBuffer;
    delete voxelManager;
    delete blockstateResolver;
    delete textureAtlas;

    delete tempTargetMS;
    delete tempTarget;
}
} // namespace openminecraftshell::renderer
