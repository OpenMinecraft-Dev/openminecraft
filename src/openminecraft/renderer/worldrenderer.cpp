#include "openminecraft-shell/renderer/worldrenderer.hpp"

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/geometric.hpp"
#include "openminecraft-shell/data/block/om_block_registery.hpp"
#include "openminecraft-shell/data/block/om_blockstate_registry.hpp"
#include "openminecraft-shell/data/block/om_blockstate_resolver.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft-shell/data/om_model_precompiler.hpp"
#include "openminecraft-shell/data/om_textureatlas.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include "openminecraft/world/om_world_chunkmanager.hpp"

#include <chrono>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <utility>

using namespace openminecraft::renderer::common;

namespace openminecraftshell::renderer
{
OMWorldRenderer::OMWorldRenderer(OMRenderer *renderer, std::shared_ptr<basics::OMCamera> camera,
                                 std::shared_ptr<OMChunkManager<16>> chunkManager)
    : OMRendererHandler(renderer), camera(std::move(camera)), logger("OMWorldRenderer", this), renderer(renderer)
{
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

    tempTarget = new wrap::OMRendererTempTarget(renderer);
    tempTarget->construct(renderer->getExtent());

    textureAtlas = new data::OMTextureAtlas("/external", renderer);

    voxelHandler = new data::OMModelPrecompiler("/external", textureAtlas);
    blockstateResolver = new data::block::OMBlockstateResolver("/external", *voxelHandler);

    blockstateResolver->resolve(data::OMIdentifier("minecraft:air"));
    blockstateResolver->buildModel(data::OMIdentifier("minecraft:air"), {});

    for (auto &p : data::block::blockRegistery.nameToId)
    {
        if (p.first.namesp != "minecraft" || p.first.path != "air")
        {
            blockstateResolver->resolve(p.first);
        }
    }

    for (auto &p : data::block::blockstateRegistry.nameToId)
    {
        auto &reg = data::block::blockstateRegistry.getRegistry(p.first);
        if (reg.block.namesp != "minecraft" || reg.block.path != "air")
        {
            blockstateResolver->buildModel(reg.block, reg.state);
        }
    }
    textureAtlas->build();

    voxelManager = new wrap::OMVoxelManager(
        renderer, tempTarget->target, textureAtlas->texture, textureAtlas->textureSecondary, chunkManager,
        [&]() -> void { record(); }, this->voxelHandler,
        [&](uint32_t v, uint64_t cx, uint64_t cy, uint64_t cz, int x, int y, int z) -> uint32_t {
            const auto &reg = data::block::blockstateRegistry.idToRegistry[v];
            uint64_t h = (cx * 16 + x) * 341873128712L + (cy * 16 + y) * 132897987541L + cz * 16 + z + 1L;
            h ^= h >> 16;
            return blockstateResolver->fetchModel(reg.block, reg.state, h);
        });

    voxelManager->pipeline->bindInput(0, cameraBuffer);
    voxelManager->debugPipeline->bindInput(0, cameraBuffer);
    voxelManager->complexPipeline->bindInput(0, cameraBuffer);
    voxelManager->translucentPipeline->bindInput(0, cameraBuffer);
    voxelManager->translucentComplexPipeline->bindInput(0, cameraBuffer);
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
    voxelManager->submit(renderer->fetchTask("voxel")->clearColor({0.198f, 0.371f, 1.0f, 1.0f})->clearDepth(0.0f),
                         tempTarget);
}

void OMWorldRenderer::afterFrame()
{
}

void OMWorldRenderer::submitTasks()
{
    tempTarget->construct(renderer->getExtent());

    renderer->createTask("voxel");

    record();
}
OMWorldRenderer::~OMWorldRenderer()
{
    delete voxelModelBuffer;
    delete uniformBuffer;
    delete cameraBuffer;
    delete lightingBuffer;
    delete voxelManager;
    delete blockstateResolver;
    delete textureAtlas;

    delete tempTarget;
}
} // namespace openminecraftshell::renderer
