#include "openminecraft-shell/network/om_protocol_minecraft.hpp"
#include "openminecraft/network/om_network_packet.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace openminecraftshell::network
{
static inline auto readVarInt(std::shared_ptr<std::istream> istr) -> int
{
    int value = 0;
    int position = 0;

    while (true)
    {
        char cb = 0;
        istr->read(&cb, 1);
        value |= (cb & 0x7f) << position;

        if ((cb & 0x80) == 0)
        {
            break;
        }

        position += 7;
    }

    return value;
}

void OMProtocolMinecraft::write(openminecraft::network::OMNetworkPacket pck)
{
    openminecraft::network::OMNetworkPacket pckid;
    pckid.writeVarInt(pck.id);

    openminecraft::network::OMNetworkPacket pcklength;
    pcklength.writeVarInt(pck.datalen() + pckid.datalen());

    out->write(pcklength.data(), pcklength.datalen());
    out->write(pckid.data(), pckid.datalen());
    out->write(pck.data(), pck.datalen());
}
auto OMProtocolMinecraft::read() -> openminecraft::network::OMNetworkPacket
{
    auto l = readVarInt(in);
    if (!l)
    {
        throw std::logic_error("empty packet!");
    }
    std::vector<uint8_t> data;
    data.resize(l);
    in->read(reinterpret_cast<char *>(data.data()), l);
    auto p = openminecraft::network::OMNetworkPacket().assign(data.begin(), data.end());
    p.id = p.readVarInt();
    return p;
}
} // namespace openminecraftshell::network