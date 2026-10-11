#include "openminecraft-shell/network/om_protocol_minecraft.hpp"
#include "openminecraft/network/om_network_packet.hpp"
#include "openminecraft/specs/zlib/om_zlib_inflate.hpp"
#include "openminecraft/specs/zlib/om_zlib_inflaterstream.hpp"
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
// TODO: zlib compression
void OMProtocolMinecraft::write(openminecraft::network::OMNetworkPacket pck)
{
    openminecraft::network::OMNetworkPacket pckid;
    pckid.writeVarInt(pck.id);

    if (compression)
    {
        openminecraft::network::OMNetworkPacket pckclength;
        pckclength.writeVarInt(0);

        openminecraft::network::OMNetworkPacket pcklength;
        pcklength.writeVarInt(pck.datalen() + pckid.datalen() + pckclength.datalen());

        out->write(pcklength.data(), pcklength.datalen());
        out->write(pckclength.data(), pckclength.datalen());
        out->write(pckid.data(), pckid.datalen());
        out->write(pck.data(), pck.datalen());
    }
    else
    {
        openminecraft::network::OMNetworkPacket pcklength;
        pcklength.writeVarInt(pck.datalen() + pckid.datalen());

        out->write(pcklength.data(), pcklength.datalen());
        out->write(pckid.data(), pckid.datalen());
        out->write(pck.data(), pck.datalen());
    }
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

    if (compression)
    {
        auto comlength = p.readVarInt();
        if (comlength)
        {
            std::vector<uint8_t> datade = {};
            openminecraft::specs::zlib::OMZLibInflater inf(
                [&](uint8_t *p, uint64_t length) { datade.insert(datade.end(), p, p + length); });
            inf.input(reinterpret_cast<uint8_t *>(p.restData()), p.restDatalen());

            p = openminecraft::network::OMNetworkPacket().assign(datade.begin(), datade.end());
        }
    }
    p.id = p.readVarInt();
    return p;
}
} // namespace openminecraftshell::network