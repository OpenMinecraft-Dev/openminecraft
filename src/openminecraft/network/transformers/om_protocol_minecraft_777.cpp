#include "openminecraft-shell/network/transformers/om_protocol_minecraft_777.hpp"
#include "openminecraft-shell/network/transformers/om_protocol_minecraft_general.hpp"

namespace openminecraftshell::network::transformers
{
void OMProtocolMinecraftTransformer777::sendLoginHello(std::string name, openminecraft::util::OMUuid uuid)
{
    protocol.write(openminecraft::network::OMNetworkPacket()
                       .packetId(0x00)
                       .writeUtf8WithLength(name)
                       .write<openminecraft::util::OMUuid>(uuid));
}

void OMProtocolMinecraftTransformer777::handleRest(OMProtocolMinecraftState state,
                                                   openminecraft::network::OMNetworkPacket &pck)
{
    switch (state)
    {
    case Login:
        switch (pck.id)
        {
        case 0x01: {
            pck.readUtf8WithLength();
            auto pk = pck.readPrefixArray<uint8_t>();
            auto vtoken = pck.readPrefixArray<uint8_t>();
            bool shouldAuth = pck.read<uint8_t>();

            logger.debug("{} {}", pk.size(), vtoken.size());
            break;
        }
        case 0x03: {
            auto thr = pck.readVarInt();
            logger.debug("compress threshould {}", thr);
            break;
        }
        default:
            break;
        }
        break;
    case Handshake:
    case Status:
    case Configure:
    case Play:
        break;
    }
    OMProtocolMinecraftTransformerGeneral::handleRest(state, pck);
}
} // namespace openminecraftshell::network::transformers