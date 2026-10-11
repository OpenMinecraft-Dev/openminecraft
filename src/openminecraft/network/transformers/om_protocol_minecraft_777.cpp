#include "openminecraft-shell/network/transformers/om_protocol_minecraft_777.hpp"
#include "openminecraft-shell/network/om_protocol_minecraft.hpp"
#include "openminecraft-shell/network/transformers/om_protocol_minecraft_general.hpp"
#include "openminecraft/network/om_network_packet.hpp"
#include "openminecraft/util/om_util_uuid.hpp"
#include <tuple>
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
            // pck.debug(64);
            logger.debug(pck.readUtf8WithLength());
            auto reg = pck.readVarInt();
            for (int i = 0; i < reg; ++i)
            {
                logger.debug(pck.readUtf8WithLength());
                auto havenbt = pck.read<uint8_t>();
                if (havenbt)
                {
                    pck.debug();

                    return;
                }
            }
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
        break;
    }

    pck.debug(64);
    OMProtocolMinecraftTransformerGeneral::handleRest(state, pck);
}
} // namespace openminecraftshell::network::transformers