#include "openminecraft/network/om_network_packet.hpp"
#include "openminecraft/network/om_network_packetstream.hpp"
#include <istream>
#include <memory>

namespace openminecraft::network
{
auto OMNetworkPacket::asStream() -> std::shared_ptr<std::istream>
{
    return std::make_shared<OMNetworkPacketIStream>(*this);
}
} // namespace openminecraft::network