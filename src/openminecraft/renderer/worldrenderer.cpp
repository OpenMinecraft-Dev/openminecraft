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
#include "openminecraft/renderer/common/animation/om_animation_multitimeline.hpp"
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
#include <glm/glm.hpp>
#include <memory>
#include <utility>
#include <vector>

using namespace openminecraft::renderer::common;

namespace openminecraftshell::renderer
{
class OMWorldColorManager : public wrap::OMVoxelColorManager
{
  private:
    animation::OMAnimationMultiTimeline<24000, glm::vec3> fogColorTimeline =
        animation::OMAnimationMultiTimeline<24000, glm::vec3>()
            .timeline(animation::OMAnimationTimelineValue<24000, glm::vec3>().append(
                          0, glm::vec3(0.752941f, 0.847059f, 1.0f)),
                      animation::Add)
            .timeline(animation::OMAnimationTimelineValue<24000, glm::vec3>()
                          .append(133, glm::vec3(1.0f))
                          .append(11867, glm::vec3(1.0f))
                          .append(13670, glm::vec3(0.06f, 0.06f, 0.09f))
                          .append(22330, glm::vec3(0.06f, 0.06f, 0.09f)),
                      animation::Multiply);

    animation::OMAnimationMultiTimeline<24000, glm::vec3> skyColorTimeline =
        animation::OMAnimationMultiTimeline<24000, glm::vec3>()
            .timeline(animation::OMAnimationTimelineValue<24000, glm::vec3>().append(0, glm::vec3(0.4706, 0.6549, 1.0)),
                      animation::Add)
            .timeline(animation::OMAnimationTimelineValue<24000, glm::vec3>()
                          .append(133, glm::vec3(1.0f))
                          .append(11867, glm::vec3(1.0f))
                          .append(13670, glm::vec3(0.0f))
                          .append(22330, glm::vec3(0.0f)),
                      animation::Multiply);

    animation::OMAnimationMultiTimeline<24000, glm::vec4> colorColorTimeline =
        animation::OMAnimationMultiTimeline<24000, glm::vec4>()
            .timeline(
                animation::OMAnimationTimelineValue<24000, glm::vec4>().append(0, glm::vec4(1.0f, 1.0f, 1.0f, 0.8f)),
                animation::Add)
            .timeline(animation::OMAnimationTimelineValue<24000, glm::vec4>()
                          .append(133, glm::vec4(1.0f))
                          .append(11867, glm::vec4(1.0f))
                          .append(13670, glm::vec4(0.1f, 0.1f, 0.15f, 1.0f))
                          .append(22330, glm::vec4(0.1f, 0.1f, 0.15f, 1.0f)),
                      animation::Multiply);

    animation::OMAnimationMultiTimeline<24000, glm::vec3> skylightColorTimeline =
        animation::OMAnimationMultiTimeline<24000, glm::vec3>()
            .timeline(animation::OMAnimationTimelineValue<24000, glm::vec3>().append(0, glm::vec3(1.0)), animation::Add)
            .timeline(animation::OMAnimationTimelineValue<24000, glm::vec3>()
                          .append(133, glm::vec3(1.0f))
                          .append(11867, glm::vec3(1.0f))
                          .append(13670, glm::vec3(0.48f, 0.48f, 1.0f))
                          .append(22330, glm::vec3(0.48f, 0.48f, 1.0f)),
                      animation::Multiply);

    animation::OMAnimationTimelineValue<24000, float> sunAngle =
        animation::OMAnimationTimelineValue<24000, float>().append(6000, 360.0f).append(6000, 0.0f);

    animation::OMAnimationTimelineValue<24000, float> moonAngle =
        animation::OMAnimationTimelineValue<24000, float>().append(6000, 540.0f).append(6000, 180.0f);

    animation::OMAnimationTimelineValue<24000, float> skylightFactorTimeline =
        animation::OMAnimationTimelineValue<24000, float>()
            .append(133, 1.0f)
            .append(11867, 1.0f)
            .append(13670, 0.24f)
            .append(22330, 0.24f);

    animation::OMAnimationTimelineValue<24000, glm::vec4> sunRiseColor =
        animation::OMAnimationTimelineValue<24000, glm::vec4>()
            .append(71, glm::vec4(0.941176f, 0.639216f, 0.2f, 0.372549f))
            .append(310, glm::vec4(0.960784f, 0.729412f, 0.2f, 0.160784f))
            .append(565, glm::vec4(0.984314f, 0.831373f, 0.2f, 0.023529f))
            .append(730, glm::vec4(1.0f, 0.898039f, 0.2f, 0.0f))
            .append(11270, glm::vec4(1.0f, 0.898039f, 0.2f, 0.0f))
            .append(11397, glm::vec4(0.988235f, 0.847059f, 0.2f, 0.015686f))
            .append(11522, glm::vec4(0.976471f, 0.796078f, 0.2f, 0.058824f))
            .append(11690, glm::vec4(0.960784f, 0.729412f, 0.2f, 0.160784f))
            .append(11929, glm::vec4(0.941176f, 0.639216f, 0.2f, 0.372549f))
            .append(12243, glm::vec4(0.905882f, 0.529412f, 0.2f, 0.694118f))
            .append(12358, glm::vec4(0.894118f, 0.494118f, 0.2f, 0.8f))
            .append(12512, glm::vec4(0.878431f, 0.447059f, 0.2f, 0.913725f))
            .append(12613, glm::vec4(0.866667f, 0.419608f, 0.2f, 0.964706f))
            .append(12732, glm::vec4(0.854902f, 0.388235f, 0.2f, 0.996078f))
            .append(12841, glm::vec4(0.843137f, 0.360784f, 0.2f, 0.996078f))
            .append(13035, glm::vec4(0.823529f, 0.317647f, 0.2f, 0.925490f))
            .append(13252, glm::vec4(0.8f, 0.278431f, 0.2f, 0.756863f))
            .append(13775, glm::vec4(0.745098f, 0.215686f, 0.2f, 0.211765f))
            .append(13888, glm::vec4(0.733333f, 0.207843f, 0.2f, 0.121569f))
            .append(14039, glm::vec4(0.717647f, 0.2f, 0.2f, 0.035294f))
            .append(14192, glm::vec4(0.701961f, 0.2f, 0.2f, 0.0f))
            .append(21807, glm::vec4(0.698039f, 0.2f, 0.2f, 0.0f))
            .append(21961, glm::vec4(0.717647f, 0.2f, 0.2f, 0.035294f))
            .append(22112, glm::vec4(0.733333f, 0.207843f, 0.2f, 0.121569f))
            .append(22225, glm::vec4(0.745098f, 0.215686f, 0.2f, 0.211765f))
            .append(22748, glm::vec4(0.8f, 0.278431f, 0.2f, 0.756863f))
            .append(22965, glm::vec4(0.823529f, 0.317647f, 0.2f, 0.925490f))
            .append(23159, glm::vec4(0.843137f, 0.360784f, 0.2f, 0.996078f))
            .append(23272, glm::vec4(0.854902f, 0.388235f, 0.2f, 0.996078f))
            .append(23488, glm::vec4(0.878431f, 0.447059f, 0.2f, 0.913725f))
            .append(23642, glm::vec4(0.894118f, 0.494118f, 0.2f, 0.8f))
            .append(23757, glm::vec4(0.905882f, 0.529412f, 0.2f, 0.694118f));

  public:
    int tick = 0;
    auto updateGameTime(int t) -> void
    {
        tick = t % 24000;
        dirty = true;
    }
    auto getSkyDiscColor() -> glm::vec3 override
    {
        return skyColorTimeline[tick];
    }
    auto getSkyDiskRange() -> float override
    {
        return 256.0f;
    }
    auto getSkyDiskHeight() -> float override
    {
        return 16.0f;
    }
    auto getFogRange() -> glm::vec2 override
    {
        return {400, 300};
    }
    auto getBlockTint() -> glm::vec3 override
    {
        return {1.0, 0.85, 0.55};
    }
    auto getSkyLightColor() -> glm::vec3 override
    {
        return skylightColorTimeline[tick];
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
        return skylightFactorTimeline[tick];
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
        return fogColorTimeline[tick];
    }
    auto getSunriseColor() -> glm::vec4 override
    {
        return sunRiseColor[tick];
    }
    auto getSunAngle() -> float override
    {
        return sunAngle[tick];
    }
    auto getMoonAngle() -> float override
    {
        return moonAngle[tick];
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
    auto getCloudColor() -> glm::vec4 override
    {
        return colorColorTimeline[tick];
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

    sunTex = renderer->allocateTexture(32, 32, 0, Dim2, R8G8B8A8Srgb);
    specs::png::OMPngFile f;
    f.parse(vfs::fsfetch("/external/minecraft/textures/environment/celestial/sun.png"));
    sunTex->updateData(f.fetchData());
    sunTex->magFilter = Nearest;
    sunTex->minFilter = Nearest;
    sunTex->setupSampler();

    moonTex = renderer->allocateTexture(32, 32, 8, 0, Dim2Array, R8G8B8A8Srgb);
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

    cloudTex = renderer->allocateTexture(256, 256, 0, Dim2, R8G8B8A8Srgb);
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

    colorManager->updateGameTime(12000);
    voxelManager = new wrap::OMVoxelManager(
        renderer, tempTarget->target, textureAtlas->texture, textureAtlas->textureSecondary, chunkManager,
        [&](bool r) -> void { record(r); }, this->voxelHandler,
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
            ->blendFunc({One, One, One, One})
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
    // voxelManager->update(*camera);
    timeBuffer->updateData(std::array<float, 1>{
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tp).count() / 1000.0f}
                               .data());
}

void OMWorldRenderer::record(bool r)
{
    voxelManager
        ->submit(renderer->fetchTask("voxel"), tempTarget, r)
        // ->pipeline(bgPipe)->drawN(6)
        ->finishN();
}

static int gameT = 12000;
static std::chrono::steady_clock::time_point tickTp = std::chrono::steady_clock::now();

auto OMWorldRenderer::getTick() -> int
{
    return gameT;
}

void OMWorldRenderer::afterFrame()
{
    voxelManager->update(*camera);
    auto cam = camera->fetchProjMat() * camera->fetchViewMat();
    cameraBuffer->synced = true;
    cameraBuffer->updateData(&cam);

    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tickTp).count() > 50)
    {
        tickTp = std::chrono::steady_clock::now();
        textureAtlas->updateAnim();
    }

    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tickTp).count() > 1)
    {
        gameT += 2;
        colorManager->updateGameTime(gameT);
    }
}

void OMWorldRenderer::submitTasks()
{
    auto siz = renderer->getExtent();
    tempTarget->construct(siz);

    screenSizeBuffer->updateData(std::array<float, 2>{siz.x, siz.y}.data());

    renderer->createTask("voxel");

    record(true);
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
    delete voxelHandler;
}
} // namespace openminecraftshell::renderer
