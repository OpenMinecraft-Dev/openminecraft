#include <SDL3/SDL_error.h>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_mouse.h"
#include "glm/ext/vector_float3.hpp"
#include "openminecraft-shell/data/block/om_block_registry.hpp"
#include "openminecraft-shell/data/block/om_blockstate_registry.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft-shell/network/transformers/om_protocol_minecraft_777.hpp"
#include "openminecraft-shell/network/transformers/om_protocol_minecraft_general.hpp"
#include "openminecraft-shell/renderer/om_compose_renderer.hpp"
#include "openminecraft-shell/renderer/om_surface_renderer.hpp"
#include "openminecraft-shell/supplements/om_interpolation_argb.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/network/om_network_dnsquery.hpp"
#include "openminecraft/renderer/common/animation/om_animation_multitimeline.hpp"
#include "openminecraft/renderer/common/animation/om_animation_timeline_value.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/om_renderer_window.hpp"
#include "openminecraft/specs/nbt/om_nbt_reader.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <algorithm>
#include <array>
#include <boost/stacktrace/stacktrace.hpp>
#include "openminecraft/renderer/common/event/om_eventbus.hpp"
#include "openminecraft/renderer/common/event/om_eventbus_wrap.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include "openminecraft-shell/renderer/om_debug_renderer.hpp"
#include "openminecraft-shell/renderer/om_world_renderer.hpp"
#include "openminecraft/world/om_world_chunkmanager.hpp"
#include "openminecraft-shell/network/om_protocol_minecraft.hpp"

#include <SDL3/SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>
#include <vector>
#include <cmath>

#include "openminecraft-shell/om_application.hpp"

using namespace openminecraft;
using namespace openminecraft::renderer::common;
using namespace openminecraft::renderer;

namespace openminecraft::shell
{
OMApplication::OMApplication(std::vector<std::string> args) : logger("OMApplication", this), args(args)
{
}
OMApplication::~OMApplication() = default;

static void setupI18nEnv()
{
    i18n::res::registerModule("openminecraft-boot");
    i18n::res::registerModule("openminecraft-renderer");
    i18n::res::pushResourceRoot("/bootassets");
    i18n::res::load();
}

auto OMApplication::entry() -> int
{
    log::multithread::registerCurrentThreadName("engineMain");
    auto logger = std::make_shared<log::OMLogger>("boot");

    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "OpenMinecraft");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, OM_VERSION);
    SDL_SetHint(SDL_HINT_APP_NAME, "OpenMinecraft");

    SDL_SetMemoryFunctions(mem::allocator::tracedMallocSDL, mem::allocator::tracedCallocSDL,
                           mem::allocator::tracedReallocSDL, mem::allocator::tracedFreeSDL);
    setupI18nEnv();
    if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO))
    {
        logger->info("SDL Status: {}", SDL_GetError());
    }
    logger->info(i18n::res::translate("openminecraft.boot.arg"));
    for (auto a : args)
    {
        logger->info(a);
    }

    // INFO: hardware information
    logger->info("hardware / software status");
    logger->info("CPU Name: {}", openminecraft::vm::os::fetchCpuName());
    logger->info("System: {}, version {}", openminecraft::vm::os::fetchSystemName(),
                 openminecraft::vm::os::fetchSystemVersion());
    logger->info("Total memory: {} bytes", openminecraft::vm::os::fetchMemoryTotal());

    OMBackend bk = Vulkan;
    if (args.size() >= 2)
    {
        if (args[1] == "help")
        {
            logger->info("OpenMinecraft Demo App {}-{}", OM_VERSION, OM_VERSION_CHANNEL);
            logger->info("usage: [backend]");
            logger->info("possible backends:");
            logger->info("vk -> Vulkan Backend (1.2, Default)");
            logger->info("gl -> OpenGL Backend (3.3 Core Profile)");
            goto end;
        }
        else if (args[1] == "network")
        {
            networkSetup(args[2], args[3]);
            goto end;
        }
        else if (args[1] == "nbt")
        {
            specs::nbt::OMNbtReader reader(std::make_shared<std::ifstream>(args[2], std::ios::binary));
            reader.parse(false);
            goto end;
        }
        bk = args[1] == "gl" ? OpenGL : Vulkan;
    }

    data::block::registerBlocks();
    data::block::registerBlockStates();
    mainLoop(bk);

end:
    SDL_Quit();

    return 0;
}

void OMApplication::networkSetup(std::string host, std::string port)
{
    logger.info("try connect to {} {}", host, port);

    auto res = openminecraft::network::queryDns(std::string("_minecraft._tcp.") + host);
    for (const auto &r : res)
    {
        host = r.target;
        port = std::to_string(r.port);
        logger.info("=> {}:{}", host, port);
    }

    vfs::fsmountTcp(host, port, "/mcserver_conn");
    auto in = vfs::fsfetch("/mcserver_conn/connect");
    auto out = vfs::fswrite("/mcserver_conn/connect");

    struct : openminecraft::shell::network::OMProtocolMinecraftHandler
    {
        log::OMLogger *logger;
        network::OMProtocolMinecraftTransformer *transformer;

        void title(const char *name)
        {
            logger->debug("** {} **", name);
        }

        void handleStatusResponse(std::string statusJson) override
        {
            title("Status Response");
            logger->debug("json: {}", statusJson);
        }

        void handleStatusPongResponse(int64_t clientTimestamp) override
        {
            title("Pong Response");
            logger->debug("client timestamp: {}", clientTimestamp);
            logger->debug("rtt: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(
                                            std::chrono::steady_clock::now().time_since_epoch())
                                                .count() -
                                            clientTimestamp);
        }

        void handleLoginSuccess(network::OMProtocolGameProfile profile) override
        {
            title("Login Success");
            logger->debug("name: {}", profile.name);
            logger->debug("uuid: {}", profile.uuid.toString());
            logger->debug("session: {}", profile.sessionId.toString());
            logger->debug("properties: {}", profile.properties.size());

            for (const auto &property : profile.properties)
            {
                logger->debug("  property {} = {} (signature {} bytes)", property.name, property.value,
                              property.signature.size());
            }

            transformer->sendLoginAcknowledged();
        }

        void handleEnterConfiguration() override
        {
            title("Enter Configuration");
            transformer->sendConfigurationKnownPacks({});
        }

        void handleConfigurationPluginMessage(std::string channel,
                                              openminecraft::network::OMNetworkPacket &data) override
        {
            title("Configuration Plugin Message");
            logger->debug("channel: {}", channel);
            logger->debug("payload: {} bytes", data.restDatalen());
        }

        void handleConfigurationFeatureFlags(std::vector<std::string> featureFlags) override
        {
            title("Feature Flags");
            logger->debug("count: {}", featureFlags.size());

            for (const auto &flag : featureFlags)
            {
                logger->debug("  {}", flag);
            }
        }

        void handleConfigurationReportDetails(const network::OMProtocolCustomReportDetails &details) override
        {
            title("Custom Report Details");
            logger->debug("count: {}", details.details.size());

            for (const auto &detail : details.details)
            {
                logger->debug("  {} = {}", detail.title, detail.description);
            }
        }

        void handleConfigurationRegistryData(const network::OMProtocolRegistryData &data) override
        {
            title("Registry Data");
            logger->debug("registry: {}", data.registry);
            logger->debug("entries: {}", data.entries.size());

            for (const auto &entry : data.entries)
            {
                logger->debug("  {}: {}", entry.name, entry.data ? "nbt" : "none");
            }
        }

        void handleConfigurationUpdateTag(const network::OMProtocolUpdateTags &tags) override
        {
            title("Update Tags");
            logger->debug("groups: {}", tags.groups.size());

            for (const auto &group : tags.groups)
            {
                logger->debug("  group {}: {} tags", group.registry, group.tags.size());

                for (const auto &tag : group.tags)
                {
                    logger->debug("    tag {}: {} entries", tag.name, tag.entries.size());
                }
            }
        }

        void handleConfigurationFinish() override
        {
            title("Configuration Finish");
            transformer->sendConfigurationFinishAcknowledged();
        }

        void handleEnterPlay() override
        {
            title("Enter Play");
        }

        void handlePlayLogin(int entityId, bool isHardcore, std::vector<std::string> dimensionNames,
                             uint32_t maxPlayers, uint32_t viewDistance, uint32_t simulationDistance,
                             bool reducedDebugInfo, bool enableRespawnScreen, bool doLimitedCrafting,
                             uint32_t dimensionTypeId, std::string dimensionName, int64_t hashedSeed, uint32_t gameMode,
                             uint32_t previousGameMode, bool isDebug, bool isFlat, bool hasDeathLocation,
                             std::string deathDimensionName, openminecraft::network::OMPackedPos deathLocation,
                             uint32_t portalCooldown, uint32_t seaLevel, bool onlineMode, bool secureChat) override
        {
            title("Login (play)");
            logger->debug("entity id: {}", entityId);
            logger->debug("hardcore: {}", isHardcore);
            logger->debug("dimension names: {}", dimensionNames.size());

            for (const auto &name : dimensionNames)
            {
                logger->debug("  {}", name);
            }

            logger->debug("max players: {}", maxPlayers);
            logger->debug("view distance: {}", viewDistance);
            logger->debug("simulation distance: {}", simulationDistance);
            logger->debug("reduced debug info: {}", reducedDebugInfo);
            logger->debug("enable respawn screen: {}", enableRespawnScreen);
            logger->debug("limited crafting: {}", doLimitedCrafting);
            logger->debug("dimension type: {}", dimensionTypeId);
            logger->debug("dimension name: {}", dimensionName);
            logger->debug("hashed seed: {}", hashedSeed);
            logger->debug("game mode: {}", gameMode);
            logger->debug("previous game mode: {}", previousGameMode);
            logger->debug("debug: {}", isDebug);
            logger->debug("flat: {}", isFlat);
            logger->debug("death location: {} {}({}, {}, {})", hasDeathLocation, deathDimensionName, deathLocation.x(),
                          deathLocation.y(), deathLocation.z());
            logger->debug("portal cooldown: {}", portalCooldown);
            logger->debug("sea level: {}", seaLevel);
            logger->debug("online mode: {}", onlineMode);
            logger->debug("secure chat: {}", secureChat);
        }

        void handlePlayChunkDataAndLight(const network::OMProtocolChunkDataAndLight &chunk) override
        {
            title("Chunk Data And Light");
            logger->debug("chunk: ({}, {}) data: {} bytes sections: {}", chunk.chunkX, chunk.chunkZ, chunk.dataLength,
                          chunk.sections.size());
            logger->debug("heightmaps: {} block entities: {}", chunk.heightmaps.size(), chunk.blockEntities.size());
            logger->debug("light: sky {} arrays, block {} arrays", chunk.light.skyLight.size(),
                          chunk.light.blockLight.size());
            logger->debug("blocks: decoded {} / declared {}", chunk.decodedBlockCount, chunk.declaredBlockCount);

            if (chunk.valid)
            {
                logger->debug("blocks verified: decoded counts match every section");
            }
            else
            {
                logger->warn("blocks NOT verified: decoded {} != declared {}", chunk.decodedBlockCount,
                             chunk.declaredBlockCount);
            }

            if (!chunk.lightComplete)
            {
                logger->debug("light arrays skipped: {} bytes (client can recompute)", chunk.lightTrailingBytes);
            }
        }

        void handlePlaySetExperience(const network::OMProtocolSetExperience &experience) override
        {
            title("Set Experience");
            logger->debug("bar: {}", experience.experienceBar);
            logger->debug("level: {}", experience.level);
            logger->debug("total: {}", experience.totalExperience);
        }

        void handlePlaySetHealth(const network::OMProtocolSetHealth &health) override
        {
            title("Set Health");
            logger->debug("health: {}", health.health);
            logger->debug("food: {}", health.food);
            logger->debug("saturation: {}", health.foodSaturation);
        }

        void handlePlayUpdateAdvancements(const network::OMProtocolUpdateAdvancements &advancements) override
        {
            title("Update Advancements");
            logger->debug("reset: {} showed: {} added: {} removed: {} progress: {}", advancements.reset,
                          advancements.showAdvancements, advancements.added.size(), advancements.removed.size(),
                          advancements.progress.size());

            for (const auto &advancement : advancements.added)
            {
                logger->debug("  advancement {}", advancement.id);

                if (advancement.parent)
                {
                    logger->debug("    parent: {}", *advancement.parent);
                }

                for (const auto &group : advancement.requirements)
                {
                    std::string joined;

                    for (const auto &criterion : group)
                    {
                        joined += criterion + " ";
                    }

                    logger->debug("    requirement: {}", joined);
                }

                logger->debug("    telemetry: {}", advancement.sendsTelemetry);
            }

            for (const auto &removed : advancements.removed)
            {
                logger->debug("  removed {}", removed);
            }

            for (const auto &progress : advancements.progress)
            {
                logger->debug("  progress {}", progress.id);

                for (const auto &criterion : progress.criteria)
                {
                    if (criterion.date)
                    {
                        logger->debug("    {} achieved at {}", criterion.id, *criterion.date);
                    }
                    else
                    {
                        logger->debug("    {} not achieved", criterion.id);
                    }
                }
            }

            if (advancements.truncated)
            {
                logger->warn("advancements truncated: display data is not parsed yet");
            }
        }

        void handlePlayUpdateAttributes(const network::OMProtocolUpdateAttributes &attributes) override
        {
            title("Update Attributes");
            logger->debug("entity: {} attributes: {}", attributes.entityId, attributes.attributes.size());

            for (const auto &attribute : attributes.attributes)
            {
                logger->debug("  attribute {}: base {}, modifiers {}", attribute.attributeId, attribute.base,
                              attribute.modifiers.size());

                for (const auto &modifier : attribute.modifiers)
                {
                    logger->debug("    modifier {}: amount {} operation {}", modifier.id, modifier.amount,
                                  modifier.operation);
                }
            }
        }

        void handlePlaySetContainerContent(const network::OMProtocolSetContainerContent &content) override
        {
            title("Set Container Content");
            logger->debug("window: {} state: {} slots: {}", content.windowId, content.stateId, content.slots.size());

            for (std::size_t i = 0; i < content.slots.size(); ++i)
            {
                const auto &slot = content.slots[i];

                if (slot.count > 0)
                {
                    logger->debug("  slot {}: item {} x{}", i, slot.itemId, slot.count);
                }
            }

            if (content.carriedItem.count > 0)
            {
                logger->debug("carried: item {} x{}", content.carriedItem.itemId, content.carriedItem.count);
            }

            if (content.truncated)
            {
                logger->warn("container content truncated: item components are not parsed yet");
            }
        }

        void handlePlayPostEffects(const network::OMProtocolPostEffects &effects) override
        {
            title("Post Effects");
            logger->debug("count: {}", effects.effects.size());

            for (const auto &effect : effects.effects)
            {
                logger->debug("  {}", effect);
            }
        }

        void handlePlayPlayerInfoUpdate(const network::OMProtocolPlayerInfoUpdate &update) override
        {
            title("Player Info Update");
            logger->debug("actions: 0x{:02x} entries: {}", update.actions, update.entries.size());

            for (const auto &entry : update.entries)
            {
                logger->debug("  player {}", entry.uuid.toString());

                if (entry.name)
                {
                    logger->debug("    name: {}", *entry.name);
                }

                for (const auto &property : entry.properties)
                {
                    logger->debug("    property {} = {} (signed: {})", property.name, property.value,
                                  property.isSigned);
                }

                if (entry.chat)
                {
                    logger->debug("    chat session {} expires {} key {} bytes signature {} bytes",
                                  entry.chat->sessionId.toString(), entry.chat->publicKeyExpiry,
                                  entry.chat->publicKey.size(), entry.chat->keySignature.size());
                }

                if (entry.gameMode)
                {
                    logger->debug("    game mode: {}", *entry.gameMode);
                }

                if (entry.listed)
                {
                    logger->debug("    listed: {}", *entry.listed);
                }

                if (entry.latency)
                {
                    logger->debug("    latency: {} ms", *entry.latency);
                }

                if (entry.displayName)
                {
                    if (const auto *text = std::get_if<specs::nbt::NBTString>(&(*entry.displayName)->data))
                    {
                        logger->debug("    display name: {}", *text);
                    }
                    else
                    {
                        logger->debug("    display name: nbt tag type {}",
                                      static_cast<int>((*entry.displayName)->type));
                    }
                }

                if (entry.hat)
                {
                    logger->debug("    hat: {}", *entry.hat);
                }

                if (entry.listOrder)
                {
                    logger->debug("    list order: {}", *entry.listOrder);
                }
            }
        }

        void handlePlayServerData(const network::OMProtocolServerData &data) override
        {
            title("Server Data");

            if (data.motd)
            {
                if (const auto *text = std::get_if<specs::nbt::NBTString>(&data.motd->data))
                {
                    logger->debug("motd: {}", *text);
                }
                else
                {
                    logger->debug("motd: nbt tag type {}", static_cast<int>(data.motd->type));
                }
            }

            logger->debug("icon: {} bytes", data.icon ? data.icon->size() : 0);
        }

        void handlePlaySetDefaultSpawnPosition(const network::OMProtocolSetDefaultSpawnPosition &spawn) override
        {
            title("Set Default Spawn Position");
            logger->debug("dimension: {}", spawn.dimension);
            logger->debug("position: ({}, {}, {})", spawn.position.x(), spawn.position.y(), spawn.position.z());
            logger->debug("yaw: {} pitch: {}", spawn.yaw, spawn.pitch);
        }

        void handlePlaySetTime(const network::OMProtocolSetTime &time) override
        {
            title("Set Time");
            logger->debug("game time: {}", time.gameTime);
            logger->debug("clocks: {}", time.clocks.size());

            for (const auto &clock : time.clocks)
            {
                logger->debug("  clock {}: total ticks {}, partial tick {}, rate {}", clock.id, clock.totalTicks,
                              clock.partialTick, clock.rate);
            }
        }

        void handlePlayInitializeBorder(double x, double z, double oldDiameter, double newDiameter, int64_t lerpTime,
                                        int32_t portalTeleportBoundary, int32_t warningBlocks,
                                        int32_t warningTime) override
        {
            title("Initialize Border");
            logger->debug("center: ({}, {})", x, z);
            logger->debug("diameter: {} -> {}", oldDiameter, newDiameter);
            logger->debug("lerp time: {} ms", lerpTime);
            logger->debug("portal teleport boundary: {}", portalTeleportBoundary);
            logger->debug("warning blocks: {} warning time: {} s", warningBlocks, warningTime);
        }

        void handlePlayChangeDifficulty(uint8_t difficulty, bool difficultyLocked) override
        {
            title("Change Difficulty");
            logger->debug("difficulty: {}", difficulty);
            logger->debug("locked: {}", difficultyLocked);
        }

        void handlePlayPlayerAbilities(uint8_t flags, float flyingSpeed, float fieldOfViewModifier) override
        {
            title("Player Abilities");
            logger->debug("flags: 0x{:02x}", flags);
            logger->debug("flying speed: {}", flyingSpeed);
            logger->debug("field of view modifier: {}", fieldOfViewModifier);
        }

        void handlePlaySetHeldSlot(uint32_t slot) override
        {
            title("Set Held Slot");
            logger->debug("slot: {}", slot);
        }

        void handlePlayEntityEvent(int entityId, uint8_t status) override
        {
            title("Entity Event");
            logger->debug("entity id: {}", entityId);
            logger->debug("status: {}", status);
        }

        void handlePlayGameEvent(int entityId, uint8_t status) override
        {
            title("Game Event");
            logger->debug("event: {}", entityId);
            logger->debug("value: {}", status);
        }

        void handlePlayRecipeBookSettings(bool craftingRecipeBookOpen, bool craftingRecipeBookFilterActive,
                                          bool smeltingRecipeBookOpen, bool smeltingRecipeBookFilterActive,
                                          bool blastFurnaceRecipeBookOpen, bool blastFurnaceRecipeBookFilterActive,
                                          bool smokerRecipeBookOpen, bool smokerRecipeBookFilterActive) override
        {
            title("Recipe Book Settings");
            logger->debug("crafting: open {} filter {}", craftingRecipeBookOpen, craftingRecipeBookFilterActive);
            logger->debug("smelting: open {} filter {}", smeltingRecipeBookOpen, smeltingRecipeBookFilterActive);
            logger->debug("blast furnace: open {} filter {}", blastFurnaceRecipeBookOpen,
                          blastFurnaceRecipeBookFilterActive);
            logger->debug("smoker: open {} filter {}", smokerRecipeBookOpen, smokerRecipeBookFilterActive);
        }

        void handlePlayPlayerPosition(uint32_t teleportId, double x, double y, double z, double velocityX,
                                      double velocityY, double velocityZ, float yaw, float pitch, int flags) override
        {
            title("Player Position");
            logger->debug("teleport id: {}", teleportId);
            logger->debug("position: ({}, {}, {})", x, y, z);
            logger->debug("velocity: ({}, {}, {})", velocityX, velocityY, velocityZ);
            logger->debug("yaw: {} pitch: {}", yaw, pitch);
            logger->debug("flags: 0x{:02x}", flags);

            transformer->sendPlayConfirmTeleportation(teleportId, x, y, z, yaw, pitch);
        }

        void handlePlayTickingState(float rate, bool isFrozen) override
        {
            title("Ticking State");
            logger->debug("rate: {}", rate);
            logger->debug("frozen: {}", isFrozen);
        }

        void handlePlayStepTick(uint32_t ticks) override
        {
            title("Step Tick");
            logger->debug("ticks: {}", ticks);
        }

        void handlePlaySetChunkCacheCenter(int32_t centerX, int32_t centerZ) override
        {
            title("Set Chunk Cache Center");
            logger->debug("center: ({}, {})", centerX, centerZ);
        }

        void handlePlayChunkBatchStart() override
        {
            title("Chunk Batch Start");
        }
    } hnd;
    hnd.logger = &logger;
    openminecraft::shell::network::OMProtocolMinecraft protocol(in, out);
    openminecraft::shell::network::transformers::OMProtocolMinecraftTransformer777 transformer(protocol, hnd);
    hnd.transformer = &transformer;

    try
    {
        transformer.sendHandshake(host, std::stoi(port), 2);
        transformer.sendLoginHello("Coder2_", openminecraft::util::OMUuid("00000000-0000-0000-0000-000000000000"));

        while (true)
        {
            transformer.handle();
        }

        /*transformer.sendStatusRequest();

        transformer.handle();
        transformer.sendStatusPingRequest(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count());
        transformer.handle();*/

        vfs::fsumount("/mcserver_conn");
    }
    catch (std::logic_error &e)
    {
        logger.error("connection closed! {}", e.what());
    }
}

void OMApplication::mainLoop(OMBackend backend)
{
    auto logger = std::make_shared<log::OMLogger>("Test Renderer");
    try
    {
        OMWindowConfig conf = {backend, false, 240, 854, 480};
        // INFO: requires at least OpenGL 4.0 Core Profile or Vulkan 1.2
        OMWindow win({util::OMVersion(4, 0, 0, 0), util::OMVersion(1, 2, 0, 0)}, conf,
                     "/bootassets/openminecraft-renderer/shaders");

        using OMEventBusSDL = event::OMEventBus<SDL_EventType, SDL_Event>;
        OMEventBusSDL bus;
        event::OMEventBusWrap buswrap;
        auto camera = std::make_shared<basics::OMCamera>(win(), glm::vec3{-1.0f, 5.0f, -1.0f}, 45.0f, -45.0f);

        auto chunkManager = std::make_shared<world::OMChunkManager<16>>();
        int width = 16;
        int height = 16;
        int depth = 8;
        for (int cx = -width; cx < width; ++cx)
        {
            for (int cy = -depth; cy <= 0; ++cy)
            {
                for (int cz = -height; cz < height; ++cz)
                {
                    using data::block::blockStateRegistry;
                    world::OMChunk<16> cnk(cx, cy, cz);
                    cnk.setBlock(0, 1, 0,
                                 blockStateRegistry.id(data::OMIdentifier("minecraft:water[falling=false,level=1]")));
                    cnk.setBlock(0, 0, 0,
                                 blockStateRegistry.id(data::OMIdentifier("minecraft:lava[falling=false,level=0]")));
                    cnk.setBlock(0, 15, 0,
                                 blockStateRegistry.id(data::OMIdentifier("minecraft:water[falling=true,level=8]")));
                    cnk.setBlock(15, 1, 0,
                                 blockStateRegistry.id(data::OMIdentifier("minecraft:water[falling=false,level=2]")));
                    cnk.setBlock(0, 1, 15,
                                 blockStateRegistry.id(data::OMIdentifier("minecraft:water[falling=false,level=2]")));
                    cnk.setBlock(15, 1, 15,
                                 blockStateRegistry.id(data::OMIdentifier("minecraft:water[falling=false,level=3]")));
                    cnk.setBlock(15, 0, 0,
                                 blockStateRegistry.id(data::OMIdentifier("minecraft:white_stained_glass[]")));
                    cnk.setBlock(0, 1, 1, blockStateRegistry.id(data::OMIdentifier("minecraft:copper_ore[]")));
                    cnk.setBlock(2, 0, 0,
                                 blockStateRegistry.id(data::OMIdentifier("minecraft:grass_block[snowy=false]")));
                    cnk.setBlock(2, 1, 0,
                                 blockStateRegistry.id(data::OMIdentifier("minecraft:tall_grass[half=lower]")));
                    cnk.setBlock(2, 2, 0,
                                 blockStateRegistry.id(data::OMIdentifier("minecraft:tall_grass[half=upper]")));
                    cnk.setBlock(
                        1, 1, 0,
                        blockStateRegistry.id(data::OMIdentifier(
                            "minecraft:cherry_stairs[facing=east,half=bottom,shape=straight,water_logged=true]")));
                    cnk.setBlock(3, 1, 0,
                                 blockStateRegistry.id(data::OMIdentifier(
                                     "minecraft:cherry_door[facing=east,half=lower,hinge=left,open=false]")));
                    cnk.setBlock(3, 2, 0,
                                 blockStateRegistry.id(data::OMIdentifier(
                                     "minecraft:cherry_door[facing=east,half=upper,hinge=left,open=false]")));
                    cnk.setBlock(0, 2, 1,
                                 blockStateRegistry.id(data::OMIdentifier(
                                     "minecraft:cherry_hanging_sign[attached=false,rotation=3,water_logged=false]")));
                    cnk.setBlock(0, 1, 2, blockStateRegistry.id(data::OMIdentifier("minecraft:copper_ore[]")));
                    cnk.setBlock(15, 1, 1, blockStateRegistry.id(data::OMIdentifier("minecraft:copper_ore[]")));
                    cnk.setBlock(15, 1, 2, blockStateRegistry.id(data::OMIdentifier("minecraft:copper_ore[]")));
                    cnk.setBlock(15, 2, 2,
                                 blockStateRegistry.id(
                                     data::OMIdentifier("minecraft:cherry_fence_gate[facing=south,in_wall=false,"
                                                        "open=true,powered=true,water_logged=false]")));
                    cnk.setBlock(
                        15, 2, 1,
                        blockStateRegistry.id(data::OMIdentifier("minecraft:stone_pressure_plate[powered=false]")));
                    chunkManager->loadChunk(cnk);
                }
            }
        }

        bool mainScreen = true;
        buswrap.append(event::Custom, [&](event::OMEvent &e) {
            logger->debug("Received event {:x}", e.custom.flag);
            switch (e.custom.flag)
            {
            case 3:
                mainScreen = true;
                break;
            case 4:
                mainScreen = false;
                break;
            case 2:
                isRunning = false;
                break;
            default:
                break;
            }
        });

        auto hnd4 = std::make_shared<renderer::OMSurfaceRenderer>(win(), buswrap);
        auto hnd2 = std::make_shared<renderer::OMDebugRenderer>(win(), buswrap);
        auto hnd = std::make_shared<renderer::OMWorldRenderer>(win(), camera, chunkManager);
        auto hnd3 = std::make_shared<renderer::OMComposeRenderer>(win(), hnd4->internal->middleTarget,
                                                                  hnd2->internal->middleTarget, hnd->tempTarget);
        hnd2->camera = camera.get();
        hnd2->stateFetch = [&]() { return hnd->voxelManager->fetchLayerState(); };
        hnd2->chunkFetch = [&]() { return hnd->voxelManager->fetchChunkState(); };
        hnd2->gameTickFetch = [&]() { return hnd->getTick(); };
        hnd->voxelManager->fetchLayerState();
        win()->registerHandler(hnd2);
        win()->registerHandler(hnd4);
        win()->registerHandler(hnd);
        win()->registerHandler(hnd3);
        win()->baseInit();

        bool inGame = false;
        std::array<bool, 6> keystates = {false, false, false, false, false, false};
        bus.append(SDL_EVENT_WINDOW_RESIZED, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_WINDOW_METAL_VIEW_RESIZED, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_WINDOW_ENTER_FULLSCREEN, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_WINDOW_LEAVE_FULLSCREEN, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_QUIT, [&](SDL_Event &) -> void { isRunning = false; });
        bus.append(SDL_EVENT_MOUSE_MOTION, [&](SDL_Event &e) -> void {
            if (SDL_GetWindowRelativeMouseMode(reinterpret_cast<SDL_Window *>(*win)))
            {
                camera->modPitch(-e.motion.yrel * 0.15);
                camera->modYaw(e.motion.xrel * 0.15);
            }
        });
        bus.append(SDL_EVENT_FINGER_MOTION, [&](SDL_Event &e) -> void {
            camera->modPitch(-e.tfinger.dy * 0.5f * 100.0f);
            camera->modYaw(e.tfinger.dx * 0.5f * 100.0f);
        });

        glm::vec3 movingSpeed = {0.0f, 0.0f, 0.0f};
        bus.appendGeneral([&]() {
            static auto startTime = std::chrono::high_resolution_clock::now();
            const auto currentTime = std::chrono::high_resolution_clock::now();
            const float time = std::chrono::duration<float>(currentTime - startTime).count();
            startTime = currentTime;

            constexpr float TICK_RATE = 20.0f;
            constexpr float TICK_DT = 1.0f / TICK_RATE;

            constexpr float FRICTION_GROUND = 0.546f;
            constexpr float FRICTION_AIR = 0.91f;
            constexpr float ACCEL_WALK = 0.1f;
            constexpr float MAX_SPEED_TICK = 0.21585f;

            static float accumulator = 0.0f;
            accumulator += time;

            while (accumulator >= TICK_DT)
            {
                float inputX = (keystates[0] ? 1.0f : 0.0f) - (keystates[2] ? 1.0f : 0.0f);
                float inputZ = (keystates[3] ? 1.0f : 0.0f) - (keystates[1] ? 1.0f : 0.0f);
                float len = std::sqrt(inputX * inputX + inputZ * inputZ);
                if (len > 0.0f)
                {
                    inputX /= len;
                    inputZ /= len;
                }

                bool onGround = false;
                float friction = onGround ? FRICTION_GROUND : FRICTION_AIR;
                movingSpeed.x *= friction;
                movingSpeed.z *= friction;
                movingSpeed.y *= FRICTION_AIR;

                float accel = ACCEL_WALK * (onGround ? 1.0f : 0.2f);
                movingSpeed.x += inputX * accel;
                movingSpeed.z += inputZ * accel;

                float horizSpeed = std::sqrt(movingSpeed.x * movingSpeed.x + movingSpeed.z * movingSpeed.z);
                if (horizSpeed > MAX_SPEED_TICK)
                {
                    movingSpeed.x *= MAX_SPEED_TICK / horizSpeed;
                    movingSpeed.z *= MAX_SPEED_TICK / horizSpeed;
                }

                accumulator -= TICK_DT;

                float frictionY = onGround ? FRICTION_GROUND : FRICTION_AIR;
                movingSpeed.y *= frictionY;

                float accelY = ACCEL_WALK * (onGround ? 1.0f : 0.2f);

                if (keystates[5])
                {
                    movingSpeed.y = fmin(movingSpeed.y + accelY, MAX_SPEED_TICK);
                }
                else if (keystates[4])
                {
                    movingSpeed.y = fmax(movingSpeed.y - accelY, -MAX_SPEED_TICK);
                }

                movingSpeed.y = std::clamp(movingSpeed.y, -MAX_SPEED_TICK, MAX_SPEED_TICK);
            }

            camera->moveCamera(basics::OMCameraMovement::Forward, movingSpeed.x * TICK_RATE * time);
            camera->moveCamera(basics::OMCameraMovement::Right, movingSpeed.z * TICK_RATE * time);
            camera->moveCamera(basics::OMCameraMovement::Up, movingSpeed.y * TICK_RATE * time);
        });

        bus.append(SDL_EVENT_MOUSE_MOTION, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::MouseMotion;
            ev.mousemotion.x = e.motion.x;
            ev.mousemotion.y = e.motion.y;
            ev.mousemotion.button = -1;
            buswrap.handle(event::MouseMotion, ev);
        });
        bus.append(SDL_EVENT_MOUSE_BUTTON_UP, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::MouseUp;
            ev.mousebutton.x = e.button.x;
            ev.mousebutton.y = e.button.y;
            ev.mousebutton.button = e.button.button;
            buswrap.handle(event::MouseUp, ev);
        });
        bus.append(SDL_EVENT_MOUSE_BUTTON_DOWN, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::MouseDown;
            ev.mousebutton.x = e.button.x;
            ev.mousebutton.y = e.button.y;
            ev.mousebutton.button = e.button.button;
            buswrap.handle(event::MouseDown, ev);
        });
        bus.append(SDL_EVENT_MOUSE_WHEEL, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::MouseWheel;
            ev.mousewheel.x = e.wheel.x;
            ev.mousewheel.y = e.wheel.y;
            ev.mousewheel.wheelx = e.wheel.x;
            ev.mousewheel.wheely = e.wheel.y;
            buswrap.handle(event::MouseWheel, ev);
        });
        bus.append(SDL_EVENT_KEY_DOWN, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::KeyDown;
            ev.key.keycode = e.key.key;
            ev.key.modifier = e.key.mod;
            buswrap.handle(event::KeyDown, ev);
        });
        bus.append(SDL_EVENT_KEY_UP, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::KeyUp;
            ev.key.keycode = e.key.key;
            ev.key.modifier = e.key.mod;
            buswrap.handle(event::KeyUp, ev);
        });
        bus.append(SDL_EVENT_FINGER_MOTION, [&](SDL_Event &e) {
            int w, h;
            SDL_GetWindowSize(SDL_GetWindowFromEvent(&e), &w, &h);
            event::OMEvent ev;
            ev.type = event::MouseMotion;
            ev.mousemotion.x = e.tfinger.x * w;
            ev.mousemotion.y = e.tfinger.y * h;
            ev.mousemotion.button = e.tfinger.fingerID;
            buswrap.handle(event::MouseMotion, ev);
        });
        bus.append(SDL_EVENT_FINGER_UP, [&](SDL_Event &e) {
            int w, h;
            SDL_GetWindowSize(SDL_GetWindowFromEvent(&e), &w, &h);
            event::OMEvent ev;
            ev.type = event::MouseUp;
            ev.mousebutton.x = e.tfinger.x * w;
            ev.mousebutton.y = e.tfinger.y * h;
            ev.mousebutton.button = e.tfinger.fingerID;
            buswrap.handle(event::MouseUp, ev);
        });
        bus.append(SDL_EVENT_FINGER_DOWN, [&](SDL_Event &e) {
            int w, h;
            SDL_GetWindowSize(SDL_GetWindowFromEvent(&e), &w, &h);
            event::OMEvent ev;
            ev.type = event::MouseDown;
            ev.mousebutton.x = e.tfinger.x * w;
            ev.mousebutton.y = e.tfinger.y * h;
            ev.mousebutton.button = e.tfinger.fingerID;
            buswrap.handle(event::MouseDown, ev);
        });

        hnd2->node->bindEventBus(buswrap);
        hnd4->node->bindEventBus(buswrap);
        bus.append(SDL_EVENT_KEY_DOWN, [&](SDL_Event &e) -> void {
            if (e.key.repeat)
            {
                return;
            }

            switch (e.key.key)
            {
            case SDLK_ESCAPE:
                inGame = false;
                break;
            case SDLK_W:
                keystates[0] = true;
                break;
            case SDLK_A:
                keystates[1] = true;
                break;
            case SDLK_S:
                keystates[2] = true;
                break;
            case SDLK_D:
                keystates[3] = true;
                break;
            case SDLK_LSHIFT:
                keystates[4] = true;
                break;
            case SDLK_SPACE:
                keystates[5] = true;
                break;
            }
        });
        bus.append(SDL_EVENT_KEY_UP, [&](SDL_Event &e) -> void {
            switch (e.key.key)
            {
            case SDLK_W:
                keystates[0] = false;
                break;
            case SDLK_A:
                keystates[1] = false;
                break;
            case SDLK_S:
                keystates[2] = false;
                break;
            case SDLK_D:
                keystates[3] = false;
                break;
            case SDLK_LSHIFT:
                keystates[4] = false;
                break;
            case SDLK_SPACE:
                keystates[5] = false;
                break;
            }
        });
        bus.append(SDL_EVENT_MOUSE_BUTTON_DOWN, [&](SDL_Event &e) -> void {
            if (e.button.button == 1 && !mainScreen)
            {
                inGame = true;
            }
        });

        util::OMTicker ticker;
        while (isRunning)
        {
            ticker.begin();

            SDL_Event e;
            while (SDL_PollEvent(&e))
            {
                bus.handle(static_cast<SDL_EventType>(e.type), e);
            }
            SDL_SetWindowRelativeMouseMode(reinterpret_cast<SDL_Window *>(*win), inGame);

            win()->render(ticker);
        }

        hnd = nullptr;
        hnd2 = nullptr;
        hnd3 = nullptr;
        hnd4 = nullptr;
    }
    catch (OMRendererException &e)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenMinecraft Fail", e.what(), nullptr);
    }
    catch (std::runtime_error &e)
    {
        logger->fatal(e.what());
    }
}
} // namespace openminecraft::shell
