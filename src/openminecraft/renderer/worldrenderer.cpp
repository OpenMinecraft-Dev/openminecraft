#include "openminecraft-shell/renderer/worldrenderer.hpp"

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/quaternion_common.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
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
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/world/om_world_chunkmanager.hpp"

#include <array>
#include <chrono>
#include <fstream>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <utility>
#include <vector>

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
    auto getSkyDiscColor() -> glm::vec3 override
    {
        return glm::mix(glm::vec3(0.0), glm::vec3(0.4706, 0.6549, 1.0), gameTime);
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
        return glm::mix(glm::vec3(0.48f, 0.48f, 1.0f), glm::vec3(1.0), gameTime);
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
        return glm::mix(0.24f, 1.0f, gameTime);
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
        return glm::vec3(0.7529, 0.8471, 1.0) * glm::mix(glm::vec3(0.05, 0.05, 0.09), glm::vec3(1.0), gameTime);
    }
    auto getSunriseColor() -> glm::vec4 override
    {
        return {0.855, 0.388, 0.200, 0.44f};
    }
    auto getSunAngle() -> float override
    {
        return 270.0f;
    }
    auto getMoonAngle() -> float override
    {
        return 90.0f;
    }
    auto getMoonPhase() -> int override
    {
        return 5;
    }
    auto getStarOpacity() -> float override
    {
        return 0.2f;
    }
    auto getStarRotation() -> float override
    {
        return 0.0f;
    }
};
static OMWorldColorManager *colorManager = new OMWorldColorManager;
static std::chrono::steady_clock::time_point tp = {};

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

    sunTex = renderer->allocateTexture(32, 32, 0, Dim2, ColorRgba);
    specs::png::OMPngFile f;
    f.parse(vfs::fsfetch("/external/minecraft/textures/environment/celestial/sun.png"));
    sunTex->updateData(f.fetchData());
    sunTex->magFilter = Nearest;
    sunTex->minFilter = Nearest;
    sunTex->setupSampler();

    moonTex = renderer->allocateTexture(32, 32, 8, 0, Dim2Array, ColorRgba);
    int i = 0;
    for (auto p : {"new_moon", "waxing_crescent", "first_quarter", "waxing_gibbous", "full_moon", "waning_gibbous",
                   "third_quarter", "waning_crescent"})
    {
        specs::png::OMPngFile f;
        f.parse(vfs::fsfetch(fmt::format("/external/minecraft/textures/environment/celestial/moon/{}.png", p)));
        moonTex->updateData(f.fetchData(), i);
        ++i;
    }
    moonTex->magFilter = Nearest;
    moonTex->minFilter = Nearest;
    moonTex->setupSampler();

    cloudTex = renderer->allocateTexture(256, 256, 0, Dim2, ColorRgba);
    specs::png::OMPngFile f2;
    f2.parse(vfs::fsfetch("/external/minecraft/textures/environment/clouds.png"));
    cloudTex->updateData(f2.fetchData());
    cloudTex->addressModeU = Repeat;
    cloudTex->addressModeV = Repeat;
    cloudTex->magFilter = Nearest;
    cloudTex->minFilter = Nearest;
    cloudTex->setupSampler();

    std::vector<bool> stats;
    for (int x = 0; x < 256; ++x)
    {
        for (int y = 0; y < 256; ++y)
        {
            stats.push_back(reinterpret_cast<uint8_t *>(f2.fetchData())[(y * 256 + x) * 4] >= 128);
        }
    }

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

    colorManager->updateGameTime(0.4);
    voxelManager = new wrap::OMVoxelManager(
        renderer, tempTarget->target, textureAtlas->texture, textureAtlas->textureSecondary, chunkManager,
        [&]() -> void { record(); }, this->voxelHandler,
        [&](uint32_t v, uint64_t cx, uint64_t cy, uint64_t cz, int x, int y, int z) -> uint32_t {
            const auto &reg = data::block::blockstateRegistry.idToRegistry[v];
            uint64_t h = (cx * 16 + x) * 341873128712L + (cy * 16 + y) * 132897987541L + cz * 16 + z + 1L;
            h ^= h >> 16;
            return blockstateResolver->fetchModel(reg.block, reg.state, h);
        },
        colorManager, sunTex, moonTex, cloudTex, stats);

    voxelManager->bindCameraBuffer(cameraBuffer);

    auto simp = basics::OMVertexFormat();
    simp.nextGroup()->decideStruct();
    bgPipe =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("ScreenData")
            ->input(UniformBuffer)
            ->inputName("Time")
            ->output(tempTarget->target)
            ->shader(renderer->shaderManager.preprocess("demiurge/scene/scene1.vert.glsl", Vertex, GLSLSource, simp))
            ->shader(renderer->shaderManager.preprocess("demiurge/scene/scene1.frag.glsl", Fragment, GLSLSource, simp))
            ->format(simp)
            ->blend(false)
            ->depth(false, false)
            ->buildN();
    screenSizeBuffer = renderer->allocateBuffer(Uniform, sizeof(float) * 2);
    timeBuffer = renderer->allocateBuffer(Uniform, sizeof(float));

    bgPipe->bindInput(0, screenSizeBuffer);
    bgPipe->bindInput(1, timeBuffer);
    tp = std::chrono::steady_clock::now();
}

void OMWorldRenderer::beforeFrame()
{
    voxelManager->update(*camera);
    timeBuffer->updateData(std::array<float, 1>{
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tp).count() / 1000.0f}
                               .data());
}

void OMWorldRenderer::record()
{
    voxelManager
        ->submit(renderer->fetchTask("voxel"), tempTarget)
        // ->pipeline(bgPipe)->drawN(6)
        ->finishN();
}

static int gameT = 0;

void OMWorldRenderer::afterFrame()
{
    auto cam = camera->fetchProjMat() * camera->fetchViewMat();
    cameraBuffer->updateData(&cam);
}

void OMWorldRenderer::submitTasks()
{
    auto siz = renderer->getExtent();
    tempTarget->construct(siz);

    screenSizeBuffer->updateData(std::array<float, 2>{siz.x, siz.y}.data());

    renderer->createTask("voxel");

    record();
}
OMWorldRenderer::~OMWorldRenderer()
{
    delete bgPipe;
    delete screenSizeBuffer;
    delete timeBuffer;

    delete uniformBuffer;
    delete cameraBuffer;
    delete voxelManager;
    delete blockstateResolver;
    delete textureAtlas;
    delete sunTex;
    delete moonTex;
    delete cloudTex;

    delete tempTarget;
}
} // namespace openminecraftshell::renderer
