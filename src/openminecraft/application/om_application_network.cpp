#include "openminecraft-shell/om_application.hpp"
#include "openminecraft/network/om_network_dnsquery.hpp"
#include "openminecraft/network/om_network_packet.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft-shell/network/transformers/om_protocol_minecraft_777.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace openminecraft;
using namespace openminecraft::renderer::common;
using namespace openminecraft::renderer;

namespace openminecraft::shell
{
// INFO: debug helper: reads exports/blockstates.txt (id<TAB>minecraft:block[props])
// and resolves the global block state ids sent by the server into readable names.
static auto blockStateTable() -> const std::vector<std::string> &
{
    static const std::vector<std::string> table = []() {
        std::vector<std::string> result;
        std::ifstream in("exports/blockstates.txt");
        std::string line;

        while (std::getline(in, line))
        {
            const auto tab = line.find('\t');
            if (tab == std::string::npos)
            {
                continue;
            }

            const int id = std::atoi(line.substr(0, tab).c_str());
            if (id < 0)
            {
                continue;
            }

            if (static_cast<int>(result.size()) <= id)
            {
                result.resize(id + 1);
            }

            result[id] = line.substr(tab + 1);
        }

        return result;
    }();

    return table;
}

static auto describeBlockState(int id) -> std::string
{
    const auto &table = blockStateTable();

    if (id >= 0 && id < static_cast<int>(table.size()) && !table[id].empty())
    {
        return table[id];
    }

    return fmt::format("<unknown id {}>", id);
}

static auto guessSection(const std::vector<std::pair<std::string, uint32_t>> &top) -> std::string
{
    if (top.empty())
    {
        return "empty";
    }

    auto blockName = [](const std::string &full) {
        const auto bracket = full.find('[');
        return bracket == std::string::npos ? full : full.substr(0, bracket);
    };

    // Take the three most common block names
    std::vector<std::pair<std::string, double>> main;
    for (size_t i = 0; i < top.size() && i < 3; ++i)
    {
        main.emplace_back(blockName(top[i].first), 100.0 * top[i].second / 4096.0);
    }

    auto has = [&](const std::string &name, double minPct) {
        for (const auto &p : main)
        {
            if (p.first == name && p.second >= minPct)
            {
                return true;
            }
        }
        return false;
    };

    auto contains = [&](const char *key, double minPct) {
        for (const auto &p : main)
        {
            if (p.second >= minPct && p.first.find(key) != std::string::npos)
            {
                return true;
            }
        }
        return false;
    };

    const double firstPct = main.empty() ? 0.0 : main[0].second;
    const std::string &first = main.empty() ? std::string() : main[0].first;

    if (firstPct >= 60.0 && first == "minecraft:air")
    {
        return "air (sky or cave)";
    }
    if (has("minecraft:lava", 5.0) || has("minecraft:water", 20.0))
    {
        return "fluid (water/lava)";
    }
    if (has("minecraft:bedrock", 5.0))
    {
        return "bedrock layer (world bottom)";
    }
    if (has("minecraft:grass_block", 3.0) || has("minecraft:short_grass", 1.0) || has("minecraft:tall_grass", 1.0) ||
        has("minecraft:dirt", 5.0))
    {
        return "surface (grass/soil)";
    }
    if (contains("leaves", 3.0) || contains("_log", 2.0) || contains("_wood", 2.0))
    {
        return "tree canopy/logs";
    }
    if (has("minecraft:sand", 10.0) || has("minecraft:sandstone", 10.0) || has("minecraft:gravel", 10.0))
    {
        return "sand/gravel bed";
    }
    if (contains("deepslate", 20.0) || contains("stone", 20.0) || contains("tuff", 10.0) ||
        contains("andesite", 10.0) || contains("granite", 10.0) || contains("diorite", 10.0))
    {
        return "underground rock (stone/deepslate)";
    }
    if (has("minecraft:water", 1.0))
    {
        return "water-bearing area";
    }

    return "mixed";
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

    struct : network::OMProtocolMinecraftHandler
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

        void handlePlayBlockUpdate(const network::OMProtocolBlockUpdate &update) override
        {
            title("Block Update");
            logger->debug("location: ({}, {}, {}) state: {}", update.location.x(), update.location.y(),
                          update.location.z(), update.blockStateId);
        }

        void handlePlayWorldEvent(const network::OMProtocolWorldEvent &event) override
        {
            title("World Event");
            logger->debug("event: {} data: {} relative volume disabled: {}", event.event, event.data,
                          event.disableRelativeVolume);
            logger->debug("location: ({}, {}, {})", event.location.x(), event.location.y(), event.location.z());
        }

        void handlePlayUpdateEntityRotation(const network::OMProtocolUpdateEntityRotation &rotation) override
        {
            title("Update Entity Rotation");
            logger->debug("entity: {} on ground: {}", rotation.entityId, rotation.onGround);
            logger->debug("yaw: {} pitch: {}", rotation.yaw.degrees, rotation.pitch.degrees);
        }

        void handlePlaySetEquipment(const network::OMProtocolSetEquipment &equipment) override
        {
            title("Set Equipment");
            logger->debug("entity: {} entries: {}", equipment.entityId, equipment.equipment.size());

            for (const auto &entry : equipment.equipment)
            {
                logger->debug("  slot {}: item {} x{}", entry.slot, entry.item.itemId, entry.item.count);
            }
        }

        void handlePlayDamageEvent(const network::OMProtocolDamageEvent &event) override
        {
            title("Damage Event");
            logger->debug("entity: {} type: {} cause: {} direct: {}", event.entityId, event.sourceTypeId,
                          event.sourceCauseId, event.sourceDirectId);

            if (event.sourceX)
            {
                logger->debug("source position: ({}, {}, {})", *event.sourceX, *event.sourceY, *event.sourceZ);
            }
        }

        void handlePlayDisconnect(const network::OMProtocolDisconnect &disconnect) override
        {
            title("Disconnect");

            if (!disconnect.reason)
            {
                logger->debug("reason: <none>");
                return;
            }

            logger->debug("reason tag type: {}", static_cast<int>(disconnect.reason->type));

            if (const auto *text = std::get_if<openminecraft::specs::nbt::NBTString>(&disconnect.reason->data))
            {
                logger->debug("reason: {}", *text);
            }
        }

        void handlePlayChunkDataAndLight(const network::OMProtocolChunkDataAndLight &chunk) override
        {
            title("Chunk Data And Light");
            logger->debug("chunk: ({}, {}) data: {} bytes sections: {}", chunk.chunkX, chunk.chunkZ, chunk.dataLength,
                          chunk.sections.size());
            logger->debug("heightmaps: {} block entities: {}", chunk.heightmaps.size(), chunk.blockEntities.size());
            logger->debug("light: {} arrays, sky mask bits {}, block mask bits {}", chunk.light.skyLight.size(),
                          chunk.light.skyLightMask.count(), chunk.light.blockLightMask.count());
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
                std::string head;

                for (auto byte : chunk.lightHead)
                {
                    head += fmt::format("{:02x} ", byte);
                }

                logger->warn("light data incomplete: {} trailing bytes, head: {}", chunk.lightTrailingBytes, head);
            }

            // Dump every section in detail (debug only), then terminate immediately.
            // 26.3 overworld height range is -64..320, each section covers 16 blocks.
            constexpr int baseY = -64;

            for (size_t i = 0; i < chunk.sections.size(); ++i)
            {
                const auto &section = chunk.sections[i];
                const auto &bs = section.blockStates;
                const auto &bm = section.biomes;

                const int y0 = baseY + static_cast<int>(i) * 16;
                const int y1 = y0 + 15;

                logger->debug("section[{}] y={}..{} blockCount={} fluidCount={}", i, y0, y1, section.blockCount,
                              section.fluidCount);

                // ---- blockStates: full histogram ----
                std::vector<uint32_t> histogram(bs.palette.size() == 0 ? 1 : bs.palette.size(), 0);

                if (bs.bitsPerEntry == 0)
                {
                    histogram[0] = 4096;
                }
                else if (!bs.data.empty())
                {
                    const uint32_t perLong = 64u / bs.bitsPerEntry;
                    const uint64_t mask = (bs.bitsPerEntry >= 64) ? ~0ull : ((1ull << bs.bitsPerEntry) - 1);

                    for (size_t longIdx = 0; longIdx < bs.data.size(); ++longIdx)
                    {
                        const uint64_t value = static_cast<uint64_t>(bs.data[longIdx]);

                        for (uint32_t slot = 0; slot < perLong; ++slot)
                        {
                            const uint32_t entry = static_cast<uint32_t>((value >> (slot * bs.bitsPerEntry)) & mask);

                            if (entry < histogram.size())
                            {
                                ++histogram[entry];
                            }
                        }
                    }
                }

                logger->debug("  blockStates: bits={} palette={} dataLongs={}", bs.bitsPerEntry, bs.palette.size(),
                              bs.data.size());

                std::vector<std::pair<std::string, uint32_t>> distribution;
                distribution.reserve(bs.palette.size());

                for (size_t k = 0; k < bs.palette.size(); ++k)
                {
                    const uint32_t count = (k < histogram.size()) ? histogram[k] : 0;

                    if (count == 0)
                    {
                        continue;
                    }

                    distribution.emplace_back(describeBlockState(bs.palette[k]), count);
                }

                std::sort(distribution.begin(), distribution.end(),
                          [](const auto &a, const auto &b) { return a.second > b.second; });

                for (size_t k = 0; k < distribution.size(); ++k)
                {
                    const double pct = 100.0 * distribution[k].second / 4096.0;
                    logger->debug("    {:>5.1f}%  x{:<4}  {}", pct, distribution[k].second, distribution[k].first);
                }

                logger->debug("  guess: {}", guessSection(distribution));

                // ---- biomes ----
                std::string biomeList;
                for (size_t k = 0; k < bm.palette.size() && k < 8; ++k)
                {
                    biomeList += fmt::format("{}{} ", bm.palette[k], k + 1 < bm.palette.size() ? "," : "");
                }
                logger->debug("  biomes: bits={} palette={} ids=[{}]", bm.bitsPerEntry, bm.palette.size(), biomeList);
            }

            throw 0;
        }

        void handlePlayUpdateEntityPosition(const network::OMProtocolUpdateEntityPosition &position) override
        {
            title("Update Entity Position");
            logger->debug("entity: {} delta: ({}, {}, {}) on ground: {}", position.entityId, position.deltaX,
                          position.deltaY, position.deltaZ, position.onGround);
        }

        void handlePlayUpdateEntityPositionRotation(
            const network::OMProtocolUpdateEntityPositionRotation &position) override
        {
            title("Update Entity Position and Rotation");
            logger->debug("entity: {} delta: ({}, {}, {}) yaw: {} pitch: {} on ground: {}", position.entityId,
                          position.deltaX, position.deltaY, position.deltaZ, position.yaw.degrees,
                          position.pitch.degrees, position.onGround);
        }

        void handlePlayTeleportEntity(const network::OMProtocolTeleportEntity &teleport) override
        {
            title("Teleport Entity");
            logger->debug("entity: {} path type: {}", teleport.entityId, teleport.pathType);

            if (teleport.pathType == 0)
            {
                logger->debug("target: ({}, {}, {})", teleport.endX, teleport.endY, teleport.endZ);
            }
            else
            {
                logger->debug("steps: {}", teleport.steps.size());

                for (const auto &step : teleport.steps)
                {
                    logger->debug("  step ({}, {}, {}) tick +{}", step.x, step.y, step.z, step.tickOffset);
                }
            }

            logger->debug("yaw: {} pitch: {} on ground: {}", teleport.yaw, teleport.pitch, teleport.onGround);
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

                for (const auto &component : slot.components.added)
                {
                    switch (component.type)
                    {
                    case network::OMDataComponentHandler::Damage:
                        logger->debug("  slot {}: damage {}", i, component.intValue);
                        break;
                    case network::OMDataComponentHandler::CustomName:
                    case network::OMDataComponentHandler::ItemName:
                        logger->debug("  slot {}: name {}", i, component.stringValue);
                        break;
                    case network::OMDataComponentHandler::Rarity:
                        logger->debug("  slot {}: rarity {}", i, component.intValue);
                        break;
                    case network::OMDataComponentHandler::Compostable:
                        logger->debug("  slot {}: compostable layers {}", i,
                                      component.compostLayers.isReference
                                          ? component.compostLayers.key
                                          : std::to_string(component.compostLayers.value));
                        break;
                    case network::OMDataComponentHandler::BrewingFuel:
                        logger->debug("  slot {}: brewing fuel uses {} speed {}", i,
                                      component.brewingUses.isReference ? component.brewingUses.key
                                                                        : std::to_string(component.brewingUses.value),
                                      component.brewingSpeedMultiplier.isReference
                                          ? component.brewingSpeedMultiplier.key
                                          : std::to_string(component.brewingSpeedMultiplier.value));
                        break;
                    case network::OMDataComponentHandler::CookingFuel:
                        logger->debug("  slot {}: cooking fuel burn {} speed {}", i,
                                      component.cookingBurnTime.isReference
                                          ? component.cookingBurnTime.key
                                          : std::to_string(component.cookingBurnTime.value),
                                      component.cookingSpeedMultiplier.isReference
                                          ? component.cookingSpeedMultiplier.key
                                          : std::to_string(component.cookingSpeedMultiplier.value));
                        break;
                    case network::OMDataComponentHandler::MobVisibility:
                        logger->debug("  slot {}: mob visibility {} entity types {}", i, component.floatValue,
                                      component.idSet.ids.size());
                        break;
                    case network::OMDataComponentHandler::VillagerFood:
                        logger->debug("  slot {}: villager food nutrition {}", i, component.intValue);
                        break;
                    case network::OMDataComponentHandler::SignTextFront:
                    case network::OMDataComponentHandler::SignTextBack:
                        logger->debug("  slot {}: sign text color {} glowing {} lines {}", i, component.intValue,
                                      component.boolValue, component.signMessages.size());
                        break;
                    case network::OMDataComponentHandler::BlocksAttacks:
                        logger->debug("  slot {}: blocks attacks delay {} reductions {}", i,
                                      component.floatList.empty() ? 0.0f : component.floatList[0],
                                      component.blockAttackReductions.size());
                        break;
                    case network::OMDataComponentHandler::Profile:
                        logger->debug("  slot {}: profile kind {} name {} properties {}", i, component.profile.kind,
                                      component.profile.username.value_or("<none>"),
                                      component.profile.properties.size());
                        break;
                    case network::OMDataComponentHandler::KineticWeapon:
                        logger->debug("  slot {}: kinetic cooldown {} conditions {} forward {} damage {}", i,
                                      component.intValue, component.kineticConditions.size(),
                                      component.floatList.empty() ? 0.0f : component.floatList[0],
                                      component.floatList.size() > 1 ? component.floatList[1] : 0.0f);
                        break;
                    case network::OMDataComponentHandler::Equippable:
                        logger->debug("  slot {}: equippable slot {} dispensable {} swappable {}", i,
                                      component.intValue, component.boolValue, component.secondBool);
                        break;
                    case network::OMDataComponentHandler::Tool:
                        logger->debug("  slot {}: tool rules {} default speed {} damage/block {} creative {} ", i,
                                      component.toolRules.size(), component.floatValue, component.intValue,
                                      component.boolValue);
                        break;
                    case network::OMDataComponentHandler::FireworkExplosion:
                        logger->debug("  slot {}: firework shape {} colors {} fade {}", i,
                                      component.fireworkExplosion.shape, component.fireworkExplosion.colors.size(),
                                      component.fireworkExplosion.fadeColors.size());
                        break;
                    case network::OMDataComponentHandler::Fireworks:
                        logger->debug("  slot {}: fireworks flight {} explosions {}", i, component.intValue,
                                      component.fireworkExplosions.size());
                        break;
                    case network::OMDataComponentHandler::PiercingWeapon:
                        logger->debug("  slot {}: piercing knockback {} dismounts {} sounds {} / {}", i,
                                      component.boolValue, component.secondBool, component.intList.size(),
                                      component.intListB.size());
                        break;
                    case network::OMDataComponentHandler::PotionContents:
                        logger->debug("  slot {}: potion {} color {} effects {}", i, component.potionId.value_or(-1),
                                      component.potionColor.value_or(-1), component.potionEffects.size());
                        break;
                    case network::OMDataComponentHandler::Consumable:
                        logger->debug("  slot {}: consumable {}s animation {} particles {} effects {}", i,
                                      component.floatValue, component.intValue, component.boolValue,
                                      component.intList.size());
                        break;
                    case network::OMDataComponentHandler::WritableBookContent:
                        logger->debug("  slot {}: writable book {} pages", i, component.bookPages.size());
                        break;
                    case network::OMDataComponentHandler::WrittenBookContent:
                        logger->debug("  slot {}: written book '{}' by {}, {} pages", i, component.stringValue,
                                      component.stringList.empty() ? "" : component.stringList[0],
                                      component.writtenBookPages.size());
                        break;
                    case network::OMDataComponentHandler::ProvidesTrimMaterial:
                        logger->debug("  slot {}: trim material {}", i,
                                      component.providesTrimMaterial.isInline()
                                          ? component.providesTrimMaterial.value->suffix
                                          : std::to_string(component.providesTrimMaterial.registryId()));
                        break;
                    case network::OMDataComponentHandler::JukeboxPlayable:
                        logger->debug("  slot {}: jukebox song {}", i,
                                      component.jukeboxSong.isInline()
                                          ? component.jukeboxSong.value->soundEvent.value
                                                ? component.jukeboxSong.value->soundEvent.value->location
                                                : std::string("<inline>")
                                          : std::to_string(component.jukeboxSong.registryId()));
                        break;
                    case network::OMDataComponentHandler::Trim:
                        logger->debug(
                            "  slot {}: trim material {} pattern {}", i,
                            component.trimMaterial.isInline() ? component.trimMaterial.value->suffix
                                                              : std::to_string(component.trimMaterial.registryId()),
                            component.trimPattern.isInline() ? component.trimPattern.value->assetName
                                                             : std::to_string(component.trimPattern.registryId()));
                        break;
                    case network::OMDataComponentHandler::LodestoneTracker:
                        logger->debug("  slot {}: lodestone has position {} tracked {}", i, component.boolValue,
                                      component.secondBool);
                        break;
                    case network::OMDataComponentHandler::BannerPatterns:
                        logger->debug("  slot {}: {} banner layers", i, component.intList.size());
                        break;
                    case network::OMDataComponentHandler::Bees:
                        logger->debug("  slot {}: {} bees", i, component.intList.size());
                        break;
                    case network::OMDataComponentHandler::UseCooldown:
                        logger->debug("  slot {}: cooldown {}s group {}", i, component.floatValue,
                                      component.optionalString.value_or("<default>"));
                        break;
                    case network::OMDataComponentHandler::AttributeModifiers:
                        for (const auto &modifier : component.attributeModifiers)
                        {
                            logger->debug("  slot {}: attribute {} {} value {} op {} slot {}", i, modifier.attributeId,
                                          modifier.id, modifier.value, modifier.operation, modifier.slot);
                        }
                        break;
                    case network::OMDataComponentHandler::UseRemainder:
                    case network::OMDataComponentHandler::SulfurCubeContent:
                        logger->debug("  slot {}: contained item {} x{}", i,
                                      component.stack != nullptr ? component.stack->itemId : -1,
                                      component.stack != nullptr ? component.stack->count : 0);
                        break;
                    case network::OMDataComponentHandler::ChargedProjectiles:
                    case network::OMDataComponentHandler::BundleContents:
                    case network::OMDataComponentHandler::Container:
                        logger->debug("  slot {}: {} contained items", i, component.stacks.size());
                        break;
                    case network::OMDataComponentHandler::CustomData:
                    case network::OMDataComponentHandler::BucketEntityData:
                    case network::OMDataComponentHandler::EntityData:
                    case network::OMDataComponentHandler::BlockEntityData:
                        logger->debug("  slot {}: nbt {}", i,
                                      component.nbt != nullptr ? static_cast<int>(component.nbt->type) : -1);
                        break;
                    default:
                        logger->debug("  slot {}: component {}", i, static_cast<int32_t>(component.type));
                        break;
                    }

                    if (component.truncated)
                    {
                        std::string head;

                        for (auto byte : component.head)
                        {
                            head += fmt::format("{:02x} ", byte);
                        }

                        logger->warn("  slot {}: component {} truncated, head: {}", i,
                                     static_cast<int32_t>(component.type), head);
                    }
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

        void handlePlaySetEntityVelocity(uint32_t entityId, double vx, double vy, double vz) override
        {
            title("Set Entity Velocity");
            logger->debug("entity: {}", entityId);
            logger->debug("velocity: {} {} {}", vx, vy, vz);
        }

        void handlePlayRemoveEntities(const network::OMProtocolRemoveEntities &entities) override
        {
            title("Remove Entities");
            logger->debug("count: {}", entities.entityIds.size());

            for (auto id : entities.entityIds)
            {
                logger->debug("  entity {}", id);
            }
        }

        void handlePlaySetHeadRotation(const network::OMProtocolSetHeadRotation &rotation) override
        {
            title("Set Head Rotation");
            logger->debug("entity: {}", rotation.entityId);
            logger->debug("head yaw: {} (raw 0x{:02x})", rotation.headYaw.degrees, rotation.headYaw.raw());
        }

        void handlePlayKeepAlive(int64_t id) override
        {
            title("Keep Alive");
            transformer->sendPlayKeepAlive(id);
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
} // namespace openminecraft::shell