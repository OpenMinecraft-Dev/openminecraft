#include "openminecraft-shell/network/transformers/om_protocol_minecraft_general.hpp"
#include "openminecraft-shell/network/om_protocol_minecraft.hpp"
#include "openminecraft/network/om_network_packet.hpp"
#include "openminecraft/util/om_util_uuid.hpp"
#include <cstdint>

namespace openminecraftshell::network::transformers
{
// Handshake, packet 0x00 (Handshake)
// structure:
// varInt protocolVersion
// prefixUtf8 host
// short port
// varInt nextState (1 for status, 2 for login)
void OMProtocolMinecraftTransformerGeneral::sendHandshake(std::string host, uint16_t port, int nextstate)
{
    protocol.write(openminecraft::network::OMNetworkPacket()
                       .packetId(0x00)
                       .writeVarInt(777)
                       .writeUtf8WithLength(host)
                       .write<int16_t>(25565)
                       .writeVarInt(nextstate));
    protocol.switchState(nextstate == 1 ? network::Status : network::Login);
}
// Status, packet 0x00 (Status Request)
void OMProtocolMinecraftTransformerGeneral::sendStatusRequest()
{
    protocol.write(openminecraft::network::OMNetworkPacket().packetId(0x00));
}
// Status, packet 0x01 (Ping Request)
// structure:
// long timestamp
void OMProtocolMinecraftTransformerGeneral::sendStatusPingRequest(int64_t t)
{
    protocol.write(openminecraft::network::OMNetworkPacket().packetId(0x01).write<int64_t>(t));
}

void OMProtocolMinecraftTransformerGeneral::handleRest(OMProtocolMinecraftState s,
                                                       openminecraft::network::OMNetworkPacket &pck)
{
    std::string state = "<unknown>";
    switch (s)
    {
    case Handshake:
        state = "Handshake";
        break;
    case Status:
        state = "Status";
        break;
    case Login:
        state = "Login";
        break;
    case Configuration:
        state = "Configuration";
        break;
    case Play:
        state = "Play";
        break;
    }
    logger.warn("unknown packet 0x{:02x} with length {} under state {}!", pck.id, pck.datalen(), state);
}

void OMProtocolMinecraftTransformerGeneral::handle()
{
    auto pck = protocol.read();
    switch (protocol.queryState())
    {
    case Status:
        switch (pck.id)
        {
            // Status Response
        case 0x00:
            handler.handleStatusResponse(pck.readUtf8WithLength());
            break;
            // Pong Response
        case 0x01:
            handler.handlePongResponse(pck.read<int64_t>());
            break;
        }
        break;
    default:
        handleRest(protocol.queryState(), pck);
        break;
    }
}
} // namespace openminecraftshell::network::transformers