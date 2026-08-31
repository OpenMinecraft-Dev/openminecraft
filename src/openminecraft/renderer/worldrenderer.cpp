#include "openminecraft-shell/renderer/worldrenderer.hpp"

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/quaternion_common.hpp"
#include "glm/ext/vector_float3.hpp"
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

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <utility>

using namespace openminecraft::renderer::common;

namespace openminecraftshell::renderer
{
class OMWorldColorManager : public wrap::OMVoxelColorManager
{
  public:
    float gameTime = 0.0f;
    auto updateGameTime(float v) -> void
    {
        gameTime = glm::clamp(v, 0.0f, 1.0f);
        dirty = true;
    }
    auto getSkyColor() -> glm::vec3 override
    {
        return glm::mix(glm::vec3{0.02, 0.03, 0.14}, glm::vec3{0.198, 0.371, 1.0}, gameTime);
    }
    auto getSkyDiscColor() -> glm::vec3 override
    {
        return glm::mix(glm::vec3{0.02, 0.02, 0.06}, glm::vec3{0.470, 0.654, 1.0}, gameTime);
    }
    auto getSkyDiskRange() -> float override
    {
        return 256.0f;
    }
    auto getSkyDiskHeight() -> float override
    {
        return -16.0f;
    }
    auto getFogRange() -> glm::vec2 override
    {
        return {0.0005, 0.0006};
    }
    auto getBlockTint() -> glm::vec3 override
    {
        return {1.0, 0.85, 0.55};
    }
    auto getSkyLightColor() -> glm::vec3 override
    {
        return glm::mix(glm::vec3{0.02, 0.03, 0.14}, glm::vec3{0.9, 0.95, 1.0}, gameTime);
    }
    auto getAmbientColor() -> glm::vec3 override
    {
        return {0.04, 0.04, 0.04};
    }
    auto getNightVisionColor() -> glm::vec3 override
    {
        return {0.7, 0.7, 0.7};
    }
    auto getBlockFactor() -> float override
    {
        return 1.0f;
    }
    auto getSkyFactor() -> float override
    {
        return 1.0f;
    }
    auto getNightVisionFactor() -> float override
    {
        return 0.0f;
    }
    auto getDarknessScale() -> float override
    {
        return 0.0f;
    }
    auto getBossOverlayWorldDarkeningFactor() -> float override
    {
        return 0.0f;
    }
    auto getBrightnessFactor() -> float override
    {
        return 1.0f;
    }
    auto getFogColor() -> glm::vec3 override
    {
        return glm::vec3{0.752, 0.84, 1.0} * glm::mix(0.1f, 1.0f, gameTime);
    }
};
static OMWorldColorManager *colorManager = new OMWorldColorManager;
OMWorldRenderer::OMWorldRenderer(OMRenderer *renderer, std::shared_ptr<basics::OMCamera> camera,
                                 std::shared_ptr<OMChunkManager<16>> chunkManager)
    : OMRendererHandler(renderer), camera(std::move(camera)), logger("OMWorldRenderer", this), renderer(renderer)
{
    format.nextGroup()->decideStruct();

    uniformBuffer = renderer->allocateBuffer(Uniform, sizeof(glm::mat4));
    auto model = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    uniformBuffer->updateData(&model);
    cameraBuffer = renderer->allocateBuffer(Uniform, sizeof(glm::mat4));

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
        },
        colorManager);

    voxelManager->bindCameraBuffer(cameraBuffer);
}

void OMWorldRenderer::beforeFrame()
{
    voxelManager->update(*camera);
}

void OMWorldRenderer::record()
{
    voxelManager->submit(renderer->fetchTask("voxel"), tempTarget);
}

static int gameT = 0;

void OMWorldRenderer::afterFrame()
{
    auto cam = camera->fetchProjMat() * camera->fetchViewMat();
    cameraBuffer->updateData(&cam);

    colorManager->updateGameTime(gameT / 24000.0f);
    gameT = (gameT + 1) % 24000;
    logger.info("{} tick", gameT);
}

void OMWorldRenderer::submitTasks()
{
    tempTarget->construct(renderer->getExtent());

    renderer->createTask("voxel");

    record();
}
OMWorldRenderer::~OMWorldRenderer()
{
    delete uniformBuffer;
    delete cameraBuffer;
    delete voxelManager;
    delete blockstateResolver;
    delete textureAtlas;

    delete tempTarget;
}
} // namespace openminecraftshell::renderer
