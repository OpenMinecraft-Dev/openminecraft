#include "openminecraft-shell/network/transformers/om_protocol_minecraft_777.hpp"
#include "openminecraft-shell/network/om_protocol_minecraft.hpp"
#include "openminecraft-shell/network/transformers/om_protocol_minecraft_general.hpp"
#include "openminecraft/network/om_network_packet.hpp"
#include "openminecraft/specs/nbt/om_nbt.hpp"
#include "openminecraft/specs/nbt/om_nbt_reader.hpp"
#include "openminecraft/util/om_util_uuid.hpp"
#include <memory>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace openminecraftshell::network::transformers
{
void OMProtocolMinecraftTransformer777::sendLoginHello(std::string name, openminecraft::util::OMUuid uuid)
{
    protocol.write(openminecraft::network::OMNetworkPacket()
                       .packetId(0x00)
                       .writeUtf8WithLength(name)
                       .write<openminecraft::util::OMUuid>(uuid));
}

void OMProtocolMinecraftTransformer777::sendLoginAcknowledged()
{
    protocol.write(openminecraft::network::OMNetworkPacket().packetId(0x03));
    protocol.switchState(network::Configuration);
    handler.handleEnterConfiguration();
}

void OMProtocolMinecraftTransformer777::sendConfigurationKnownPacks(
    std::vector<std::tuple<std::string, std::string, std::string>> pcks)
{
    auto pck = openminecraft::network::OMNetworkPacket().packetId(0x07).writeVarInt(pcks.size());
    for (const auto &p : pcks)
    {
        pck.writeUtf8WithLength(std::get<0>(p));
        pck.writeUtf8WithLength(std::get<1>(p));
        pck.writeUtf8WithLength(std::get<2>(p));
    }
    protocol.write(pck);
}

void OMProtocolMinecraftTransformer777::sendConfigurationFinishAcknowledged()
{
    protocol.write(openminecraft::network::OMNetworkPacket().packetId(0x03));
    protocol.switchState(network::Play);
    handler.handleEnterPlay();
}

void OMProtocolMinecraftTransformer777::handleRest(OMProtocolMinecraftState state,
                                                   openminecraft::network::OMNetworkPacket &pck)
{
    switch (state)
    {
    case Login:
        switch (pck.id)
        {
            // Encryption Request
        case 0x01: {
            pck.readUtf8WithLength();
            auto pk = pck.readPrefixArray<uint8_t>();
            auto vtoken = pck.readPrefixArray<uint8_t>();
            bool shouldAuth = pck.read<uint8_t>();

            logger.debug("{} {}", pk.size(), vtoken.size());
            return;
        }
            // Login Success
        case 0x02: {
            OMProtocolGameProfile profile = {};
            profile.uuid = pck.read<openminecraft::util::OMUuid>().toString();
            profile.name = pck.readUtf8WithLength();
            for (int i = 0; i < pck.readVarInt(); ++i)
            {
                profile.properties.emplace_back(pck.readUtf8WithLength(), pck.readUtf8WithLength(),
                                                pck.readUtf8WithLength());
            }
            profile.sessionId = pck.read<openminecraft::util::OMUuid>().toString();
            handler.handleLoginSucccess(profile);
            return;
        }
            // Set Compression
        case 0x03: {
            auto thr = pck.readVarInt();
            protocol.enableCompression(thr);
            logger.debug("compress threshould {} bytes", thr);
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
            handler.handleConfigurationPluginMessage(pck.readUtf8WithLength(), pck);
            return;
            // Configuration Finish
        case 0x03:
            handler.handleConfigurationFinish();
            return;
            // Feature Flags
        case 0x0d: {
            auto i = pck.readVarInt();
            std::vector<std::string> flags;
            for (int k = 0; k < i; ++k)
            {
                flags.push_back(pck.readUtf8WithLength());
            }
            handler.handleConfigurationFeatureFlags(flags);
            return;
        }
            // Registry Data
        case 0x07: {
            auto regName = pck.readUtf8WithLength();
            auto reg = pck.readVarInt();

            std::unordered_map<std::string, std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>> regItems;

            for (int i = 0; i < reg; ++i)
            {
                auto name = pck.readUtf8WithLength();
                auto havenbt = pck.read<bool>();
                std::shared_ptr<openminecraft::specs::nbt::OMNbtTag> args = nullptr;
                if (havenbt)
                {
                    openminecraft::specs::nbt::OMNbtReader r(pck.asStream());
                    args = r.parse(false);
                }

                regItems[name] = args;
            }

            handler.handleConfigurationRegistryData(regName, regItems);
            return;
        }
            // Update Tag
        case 0x0e: {
            std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> tags;
            int regCount = pck.readVarInt();
            for (int i = 0; i < regCount; ++i)
            {
                auto regName = pck.readUtf8WithLength();
                auto &reg = tags[regName];
                int tagsCount = pck.readVarInt();

                for (int ti = 0; ti < tagsCount; ++ti)
                {
                    auto &entries = reg[pck.readUtf8WithLength()];

                    int entriesCount = pck.readVarInt();
                    for (int e = 0; e < entriesCount; ++e)
                    {
                        entries.push_back(pck.readVarInt());
                    }
                }
            }
            handler.handleConfigurationUpdateTag(tags);
            return;
        }
            // Chat Report Custom
        case 0x0f: {
            auto i = pck.readVarInt();
            std::vector<std::tuple<std::string, std::string, std::string>> report;
            for (int k = 0; k < i; ++k)
            {
                report.emplace_back(pck.readUtf8WithLength(), pck.readUtf8WithLength(), pck.readUtf8WithLength());
            }
            handler.handleConfigurationReportDetails(report);
            return;
        }
        default:
            break;
        }
    case Play:
        switch (pck.id)
        {
            // Change Difficulty
        case 0x0a: {
            handler.handlePlayChangeDifficulty(pck.read<uint8_t>(), pck.read<bool>());
            return;
        }
            // Login
        case 0x32: {
            auto entityId = pck.read<int32_t>();
            bool ishardcore = pck.read<bool>();

            std::vector<std::string> dims;
            int dimC = pck.readVarInt();
            for (int i = 0; i < dimC; ++i)
            {
                dims.push_back(pck.readUtf8WithLength());
            }
            auto maxplayers = pck.readVarInt();
            auto viewDistance = pck.readVarInt();
            auto simulationDistance = pck.readVarInt();
            auto reducedDebugInfo = pck.read<bool>();
            auto enableRespawnScreen = pck.read<bool>();
            auto doLimitedCrafting = pck.read<bool>();
            auto dimType = pck.readVarInt();
            auto dimName = pck.readUtf8WithLength();
            auto hashSeed = pck.read<int64_t>();
            auto gameMode = pck.readVarInt();
            auto prevGameMode = pck.readVarInt();
            auto isDebug = pck.read<bool>();
            auto isFlat = pck.read<bool>();
            auto hasDeathLocation = pck.read<bool>();
            std::string deathDim = "";
            int64_t depthPos = 0;
            if (hasDeathLocation)
            {
                deathDim = pck.readUtf8WithLength();
                depthPos = pck.read<int64_t>();
            }
            auto portalCooldown = pck.readVarInt();
            auto seaLevel = pck.readVarInt();
            auto onlineMode = pck.read<bool>();
            auto secureChat = pck.read<bool>();

            handler.handlePlayLogin(entityId, ishardcore, dims, maxplayers, viewDistance, simulationDistance,
                                    reducedDebugInfo, enableRespawnScreen, doLimitedCrafting, dimType, dimName,
                                    hashSeed, gameMode, prevGameMode, isDebug, isFlat, hasDeathLocation, deathDim,
                                    depthPos, portalCooldown, seaLevel, onlineMode, secureChat);
            return;
        }
            // Player Abilities
        case 0x41: {
            handler.handlePlayPlayerAbilities(pck.read<uint8_t>(), pck.read<float>(), pck.read<float>());
            return;
        }
            // Set Held Slot
        case 0x6b: {
            handler.handlePlaySetHeldSlot(pck.readVarInt());
            return;
        }
        default:
            break;
        }
    }

    pck.debug(64);
    OMProtocolMinecraftTransformerGeneral::handleRest(state, pck);
    throw std::logic_error("unknown packet!");
}
} // namespace openminecraftshell::network::transformers