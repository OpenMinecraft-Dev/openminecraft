#include "openminecraft-shell/network/transformers/om_protocol_minecraft_general.hpp"
#include "openminecraft-shell/network/om_protocol_minecraft.hpp"
#include "openminecraft/network/om_network_packet.hpp"

namespace openminecraftshell::network::transformers
{
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
void OMProtocolMinecraftTransformerGeneral::sendStatusRequest()
{
    protocol.write(openminecraft::network::OMNetworkPacket().packetId(0x00));
}
void OMProtocolMinecraftTransformerGeneral::sendStatusPingRequest(int64_t t)
{
    protocol.write(openminecraft::network::OMNetworkPacket().packetId(0x01).write<int64_t>(t));
}

void OMProtocolMinecraftTransformerGeneral::sendLoginHello(std::string name, uint64_t ul, uint64_t uh)
{
    protocol.write(openminecraft::network::OMNetworkPacket()
                       .packetId(0x00)
                       .writeUtf8WithLength(name)
                       .write<int64_t>(ul)
                       .write<int64_t>(uh));
}

void OMProtocolMinecraftTransformerGeneral::handleRest(OMProtocolMinecraftState s,
                                                       openminecraft::network::OMNetworkPacket &pck)
{
    logger.warn("unknown packet 0x{:02x} with length {} under state {}!", pck.id, pck.datalen(), static_cast<int>(s));
}

void OMProtocolMinecraftTransformerGeneral::handle()
{
    auto pck = protocol.read();
    switch (protocol.queryState())
    {
    case Status:
        switch (pck.id)
        {
        case 0x00:
            handler.handleStatusResponse(pck.readUtf8WithLength());
            break;
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