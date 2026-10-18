#include "openminecraft-shell/network/transformers/om_protocol_minecraft_777.hpp"
#include "openminecraft-shell/network/om_protocol_minecraft.hpp"
#include "openminecraft-shell/network/transformers/om_protocol_minecraft_general.hpp"
#include "openminecraft/network/om_network_packet.hpp"
#include "openminecraft/util/om_util_uuid.hpp"
#include <vector>

using openminecraft::network::OMAngle;
using openminecraft::network::OMLpVec3;
using openminecraft::network::OMPackedPos;
using openminecraft::network::OMUtf8String;
using openminecraft::network::OMVarInt;
using openminecraft::network::OMVarLong;

namespace openminecraft::shell::network::transformers
{
void OMProtocolMinecraftTransformer777::sendLoginHello(std::string name, openminecraft::util::OMUuid uuid)
{
    protocol.write(openminecraft::network::OMNetworkPacket()
                       .packetId(0x00)
                       .write(OMUtf8String(name))
                       .write<openminecraft::util::OMUuid>(uuid));
}

void OMProtocolMinecraftTransformer777::sendLoginAcknowledged()
{
    protocol.write(openminecraft::network::OMNetworkPacket().packetId(0x03));
    protocol.switchState(network::Configuration);
    handler.handleEnterConfiguration();
}

void OMProtocolMinecraftTransformer777::sendConfigurationKnownPacks(std::vector<OMProtocolKnownPack> pcks)
{
    protocol.write(openminecraft::network::OMNetworkPacket().packetId(0x07).writePrefixArray(pcks));
}

void OMProtocolMinecraftTransformer777::sendConfigurationFinishAcknowledged()
{
    protocol.write(openminecraft::network::OMNetworkPacket().packetId(0x03));
    protocol.switchState(network::Play);
    handler.handleEnterPlay();
}

void OMProtocolMinecraftTransformer777::sendPlayConfirmTeleportation(uint32_t teleportID, double x, double y, double z,
                                                                     float yaw, float pitch)
{
    protocol.write(openminecraft::network::OMNetworkPacket()
                       .packetId(0x0)
                       .write<OMVarInt>(teleportID)
                       .write(x)
                       .write(y)
                       .write(z)
                       .write(yaw)
                       .write(pitch));
}

void OMProtocolMinecraftTransformer777::sendPlayKeepAlive(int64_t id)
{
    protocol.write(openminecraft::network::OMNetworkPacket().packetId(0x1c).write(id));
}

auto OMProtocolMinecraftTransformer777::componentNames() -> std::vector<std::string>
{
    // INFO: order matches the field registration order of net.minecraft.core.component.DataComponents (protocol 777 / 1.21.9)
    return {
        "minecraft:custom_data",
        "minecraft:max_stack_size",
        "minecraft:max_damage",
        "minecraft:damage",
        "minecraft:unbreakable",
        "minecraft:use_effects",
        "minecraft:custom_name",
        "minecraft:minimum_attack_charge",
        "minecraft:damage_type",
        "minecraft:item_name",
        "minecraft:item_model",
        "minecraft:lore",
        "minecraft:rarity",
        "minecraft:enchantments",
        "minecraft:can_place_on",
        "minecraft:can_break",
        "minecraft:attribute_modifiers",
        "minecraft:custom_model_data",
        "minecraft:tooltip_display",
        "minecraft:repair_cost",
        "minecraft:creative_slot_lock",
        "minecraft:enchantment_glint_override",
        "minecraft:intangible_projectile",
        "minecraft:food",
        "minecraft:consumable",
        "minecraft:use_remainder",
        "minecraft:use_cooldown",
        "minecraft:damage_resistant",
        "minecraft:tool",
        "minecraft:weapon",
        "minecraft:attack_range",
        "minecraft:enchantable",
        "minecraft:equippable",
        "minecraft:repairable",
        "minecraft:glider",
        "minecraft:tooltip_style",
        "minecraft:death_protection",
        "minecraft:blocks_attacks",
        "minecraft:piercing_weapon",
        "minecraft:kinetic_weapon",
        "minecraft:attack_animation",
        "minecraft:interact_animation",
        "minecraft:additional_trade_cost",
        "minecraft:block_transformer",
        "minecraft:villager_food",
        "minecraft:stored_enchantments",
        "minecraft:dye",
        "minecraft:dyed_color",
        "minecraft:map_id",
        "minecraft:map_decorations",
        "minecraft:map_post_processing",
        "minecraft:charged_projectiles",
        "minecraft:bundle_contents",
        "minecraft:potion_contents",
        "minecraft:potion_duration_scale",
        "minecraft:suspicious_stew_effects",
        "minecraft:writable_book_content",
        "minecraft:written_book_content",
        "minecraft:trim",
        "minecraft:debug_stick_state",
        "minecraft:entity_data",
        "minecraft:bucket_entity_data",
        "minecraft:block_entity_data",
        "minecraft:instrument",
        "minecraft:provides_trim_material",
        "minecraft:ominous_bottle_amplifier",
        "minecraft:jukebox_playable",
        "minecraft:provides_banner_patterns",
        "minecraft:recipes",
        "minecraft:lodestone_tracker",
        "minecraft:firework_explosion",
        "minecraft:fireworks",
        "minecraft:profile",
        "minecraft:note_block_sound",
        "minecraft:banner_patterns",
        "minecraft:base_color",
        "minecraft:pot_decorations",
        "minecraft:container",
        "minecraft:block_state",
        "minecraft:bees",
        "minecraft:sulfur_cube_content",
        "minecraft:lock",
        "minecraft:container_loot",
        "minecraft:break_sound",
        "minecraft:compostable",
        "minecraft:cooking_fuel",
        "minecraft:brewing_fuel",
        "minecraft:mob_visibility",
        "minecraft:villager/variant",
        "minecraft:wolf/variant",
        "minecraft:wolf/sound_variant",
        "minecraft:wolf/collar",
        "minecraft:fox/variant",
        "minecraft:salmon/size",
        "minecraft:parrot/variant",
        "minecraft:tropical_fish/pattern",
        "minecraft:tropical_fish/base_color",
        "minecraft:tropical_fish/pattern_color",
        "minecraft:mooshroom/variant",
        "minecraft:rabbit/variant",
        "minecraft:pig/variant",
        "minecraft:pig/sound_variant",
        "minecraft:cow/variant",
        "minecraft:cow/sound_variant",
        "minecraft:chicken/variant",
        "minecraft:chicken/sound_variant",
        "minecraft:zombie_nautilus/variant",
        "minecraft:frog/variant",
        "minecraft:horse/variant",
        "minecraft:painting/variant",
        "minecraft:llama/variant",
        "minecraft:axolotl/variant",
        "minecraft:cat/variant",
        "minecraft:cat/sound_variant",
        "minecraft:cat/collar",
        "minecraft:sheep/color",
        "minecraft:shulker/color",
        "minecraft:provides_pottery_pattern",
        "minecraft:sign_text_front",
        "minecraft:sign_text_back",
        "minecraft:waxed",
        "minecraft:cushion/color",
    };
}

void OMProtocolMinecraftTransformer777::handleRest(OMProtocolMinecraftState state,
                                                   openminecraft::network::OMNetworkPacket &pck)
{
    pck.setUserContext(&componentMapping);

    switch (state)
    {
    case Login:
        switch (pck.id)
        {
            // Encryption Request
        case 0x01: {
            pck.read<OMUtf8String>();
            auto pk = pck.readPrefixArray<uint8_t>();
            auto vtoken = pck.readPrefixArray<uint8_t>();
            bool shouldAuth = pck.read<uint8_t>();

            logger.debug("{} {}", pk.size(), vtoken.size());
            return;
        }
            // Login Success
        case 0x02: {
            handler.handleLoginSuccess(pck.read<OMProtocolGameProfile>());
            return;
        }
            // Set Compression
        case 0x03: {
            auto thr = pck.read<OMVarInt>();
            protocol.enableCompression(thr);
            logger.debug("compress threshold {} bytes", thr);
            return;
        }
        default:
            break;
        }
        break;
    case Handshake:
    case Status:
    case Configuration:
        switch (pck.id)
        {
            // Plugin Message
        case 0x01:
            handler.handleConfigurationPluginMessage(pck.read<OMUtf8String>(), pck);
            return;
            // Configuration Finish
        case 0x03:
            handler.handleConfigurationFinish();
            return;
            // Feature Flags
        case 0x0d: {
            handler.handleConfigurationFeatureFlags(pck.readPrefixArray<std::string>());
            return;
        }
            // Registry Data
        case 0x07: {
            handler.handleConfigurationRegistryData(pck.read<OMProtocolRegistryData>());
            return;
        }
            // Update Tag
        case 0x0e: {
            handler.handleConfigurationUpdateTag(pck.read<OMProtocolUpdateTags>());
            return;
        }
            // Chat Report Custom
        case 0x0f: {
            handler.handleConfigurationReportDetails(pck.read<OMProtocolCustomReportDetails>());
            return;
        }
        default:
            break;
        }
    case Play:
        switch (pck.id)
        {
            // Bundle Delimeter
        case 0x00: {
            return;
        }
            // Spawn Entity
        case 0x01: {
            handler.handlePlaySpawnEntity(pck.read<OMProtocolSpawnEntity>());
            return;
        }
            // Change Difficulty
        case 0x0a: {
            handler.handlePlayChangeDifficulty(pck.read<uint8_t>(), pck.read<bool>());
            return;
        }
            // Chunk Batch End
        case 0x0b: {
            handler.handlePlayChunkBatchEnd(pck.read<OMVarInt>());
            return;
        }
            // Chunk Batch Start
        case 0x0c: {
            handler.handlePlayChunkBatchStart();
            return;
        }
            // Commands
        case 0x10: {
            logger.warn("skipped Commands packet!");
            return;
        }
            // Set Container Content
        case 0x12: {
            handler.handlePlaySetContainerContent(pck.read<OMProtocolSetContainerContent>());
            return;
        }
            // Disconnect
        case 0x20: {
            handler.handlePlayDisconnect(pck.read<OMProtocolDisconnect>());
            return;
        }
            // Entity Event
        case 0x22: {
            handler.handlePlayEntityEvent(pck.read<int32_t>(), pck.read<uint8_t>());
            return;
        }
            // Teleport Entity
        case 0x23: {
            handler.handlePlayTeleportEntity(pck.read<OMProtocolTeleportEntity>());
            return;
        }
            // Game Event
        case 0x27: {
            handler.handlePlayGameEvent(pck.read<int32_t>(), pck.read<uint8_t>());
            return;
        }
            // Initialize Border
        case 0x2c: {
            auto x = pck.read<double>();
            auto z = pck.read<double>();
            auto oldDiameter = pck.read<double>();
            auto newDiameter = pck.read<double>();
            auto lerpTime = pck.read<OMVarLong>();
            auto portalTeleportBoundary = pck.read<OMVarInt>();
            auto warningBlocks = pck.read<OMVarInt>();
            auto warningTime = pck.read<OMVarInt>();

            handler.handlePlayInitializeBorder(x, z, oldDiameter, newDiameter, lerpTime, portalTeleportBoundary,
                                               warningBlocks, warningTime);
            return;
        }
            // Keep Alive
        case 0x2d: {
            handler.handlePlayKeepAlive(pck.read<int64_t>());
            return;
        }
            // Chunk Data And Light
        case 0x2e: {
            handler.handlePlayChunkDataAndLight(pck.read<OMProtocolChunkDataAndLight>());
            return;
        }
            // Update Light
        case 0x31: {
            handler.handlePlayUpdateLight(pck.read<OMProtocolUpdateLight>());
            return;
        }
            // Login
        case 0x32: {
            auto entityId = pck.read<int32_t>();
            bool ishardcore = pck.read<bool>();

            auto dims = pck.readPrefixArray<std::string>();
            auto maxplayers = pck.read<OMVarInt>();
            auto viewDistance = pck.read<OMVarInt>();
            auto simulationDistance = pck.read<OMVarInt>();
            auto reducedDebugInfo = pck.read<bool>();
            auto enableRespawnScreen = pck.read<bool>();
            auto doLimitedCrafting = pck.read<bool>();
            auto dimType = pck.read<OMVarInt>();
            auto dimName = pck.read<OMUtf8String>();
            auto hashSeed = pck.read<int64_t>();
            auto gameMode = pck.read<OMVarInt>();
            auto prevGameMode = pck.read<OMVarInt>();
            auto isDebug = pck.read<bool>();
            auto isFlat = pck.read<bool>();
            auto hasDeathLocation = pck.read<bool>();
            std::string deathDim = "";
            OMPackedPos depthPos;
            if (hasDeathLocation)
            {
                deathDim = pck.read<OMUtf8String>();
                depthPos = pck.read<OMPackedPos>();
            }
            auto portalCooldown = pck.read<OMVarInt>();
            auto seaLevel = pck.read<OMVarInt>();
            auto onlineMode = pck.read<bool>();
            auto secureChat = pck.read<bool>();

            handler.handlePlayLogin(entityId, ishardcore, dims, maxplayers, viewDistance, simulationDistance,
                                    reducedDebugInfo, enableRespawnScreen, doLimitedCrafting, dimType, dimName,
                                    hashSeed, gameMode, prevGameMode, isDebug, isFlat, hasDeathLocation, deathDim,
                                    depthPos, portalCooldown, seaLevel, onlineMode, secureChat);
            return;
        }
            // Update Entity Position
        case 0x36: {
            handler.handlePlayUpdateEntityPosition(pck.read<OMProtocolUpdateEntityPosition>());
            return;
        }
            // Update Entity Position and Rotation
        case 0x37: {
            handler.handlePlayUpdateEntityPositionRotation(pck.read<OMProtocolUpdateEntityPositionRotation>());
            return;
        }
            // Player Abilities
        case 0x41: {
            handler.handlePlayPlayerAbilities(pck.read<uint8_t>(), pck.read<float>(), pck.read<float>());
            return;
        }
            // Player Info Update
        case 0x47: {
            handler.handlePlayPlayerInfoUpdate(pck.read<OMProtocolPlayerInfoUpdate>());
            return;
        }
            // Player Position
        case 0x49: {
            handler.handlePlayPlayerPosition(
                pck.read<OMVarInt>(), pck.read<double>(), pck.read<double>(), pck.read<double>(), pck.read<double>(),
                pck.read<double>(), pck.read<double>(), pck.read<float>(), pck.read<float>(), pck.read<int32_t>());
            return;
        }
            // Recipe Book Add
        case 0x4b: {
            logger.warn("skipped Recipe Book Add packet!");
            return;
        }
            // Remove Entities
        case 0x4e: {
            handler.handlePlayRemoveEntities(pck.read<OMProtocolRemoveEntities>());
            return;
        }
            // Recipe Book Settings
        case 0x4d: {
            handler.handlePlayRecipeBookSettings(pck.read<bool>(), pck.read<bool>(), pck.read<bool>(), pck.read<bool>(),
                                                 pck.read<bool>(), pck.read<bool>(), pck.read<bool>(),
                                                 pck.read<bool>());
            return;
        }
            // Post Effects
        case 0x53: {
            handler.handlePlayPostEffects(pck.read<OMProtocolPostEffects>());
            return;
        }
            // Set Head Rotation
        case 0x55: {
            handler.handlePlaySetHeadRotation(pck.read<OMProtocolSetHeadRotation>());
            return;
        }
            // Server Data
        case 0x58: {
            handler.handlePlayServerData(pck.read<OMProtocolServerData>());
            return;
        }
            // Set Chunk Cache Center
        case 0x60: {
            handler.handlePlaySetChunkCacheCenter(pck.read<OMVarInt>(), pck.read<OMVarInt>());
            return;
        }
            // Set Default Spawn Position
        case 0x63: {
            handler.handlePlaySetDefaultSpawnPosition(pck.read<OMProtocolSetDefaultSpawnPosition>());
            return;
        }
            // Set Entity Metadata
        case 0x65: {
            logger.debug("skipped Set Entity Metadata packet!");
            return;
        }
            // Set Entity Velocity
        case 0x67: {
            auto entityId = pck.read<OMVarInt>();
            auto v = pck.read<OMLpVec3>();
            handler.handlePlaySetEntityVelocity(entityId, v.x, v.y, v.z);
            return;
        }
            // Set Experience
        case 0x69: {
            handler.handlePlaySetExperience(pck.read<OMProtocolSetExperience>());
            return;
        }
            // Set Health
        case 0x6a: {
            handler.handlePlaySetHealth(pck.read<OMProtocolSetHealth>());
            return;
        }
            // Set Held Slot
        case 0x6b: {
            handler.handlePlaySetHeldSlot(pck.read<OMVarInt>());
            return;
        }
            // Set Time
        case 0x73: {
            handler.handlePlaySetTime(pck.read<OMProtocolSetTime>());
            return;
        }
            //
            // Ticking State
        case 0x82: {
            handler.handlePlayTickingState(pck.read<float>(), pck.read<bool>());
            return;
        }
            // Step Tick
        case 0x83: {
            handler.handlePlayStepTick(pck.read<OMVarInt>());
            return;
        }
            // Update Advancements
        case 0x85: {
            handler.handlePlayUpdateAdvancements(pck.read<OMProtocolUpdateAdvancements>());
            return;
        }
            // Update Attributes
        case 0x86: {
            handler.handlePlayUpdateAttributes(pck.read<OMProtocolUpdateAttributes>());
            return;
        }
            // Update Recipes
        case 0x88: {
            logger.warn("skipped Update Recipes packet!");
            return;
        }
        default:
            break;
        }
    }

    pck.debug(64);
    OMProtocolMinecraftTransformerGeneral::handleRest(state, pck);
}
} // namespace openminecraft::shell::network::transformers