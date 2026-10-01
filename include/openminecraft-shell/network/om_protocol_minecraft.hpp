#ifndef OM_PROTOCOL_MINECRAFT_HPP
#define OM_PROTOCOL_MINECRAFT_HPP

#include "openminecraft/network/protocol/coding/om_coding.hpp"
#include <memory>
namespace openminecraftshell::network
{
enum OMProtocolMinecraftState
{
    Handshake,
    Status,
    Login,
    Configure,
    Play
};
class OMProtocolMinecraftHandler
{
  public:
    OMProtocolMinecraftHandler() = default;
};

class OMProtocolMinecraft : public openminecraft::network::protocol::OMCodingBase<OMProtocolMinecraftHandler>
{
  public:
    OMProtocolMinecraft(std::shared_ptr<std::istream> in, std::shared_ptr<std::ostream> out,
                        OMProtocolMinecraftHandler &handler)
        : OMCodingBase(in, out, handler)
    {
    }

    void write(openminecraft::network::OMNetworkPacket) override;
    auto read() -> openminecraft::network::OMNetworkPacket override;
};
} // namespace openminecraftshell::network

#endif