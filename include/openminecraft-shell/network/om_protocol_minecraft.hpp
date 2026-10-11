#ifndef OM_PROTOCOL_MINECRAFT_HPP
#define OM_PROTOCOL_MINECRAFT_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/network/protocol/coding/om_coding.hpp"
#include "openminecraft/util/om_util_uuid.hpp"
#include <functional>
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

    std::function<void(std::string)> handleStatusResponse = [](std::string) {};
    std::function<void(int64_t)> handlePongResponse = [](int64_t) {};
};

class OMProtocolMinecraft : public openminecraft::network::protocol::OMCodingBase
{
  private:
    OMProtocolMinecraftState state = Handshake;

  public:
    OMProtocolMinecraft(std::shared_ptr<std::istream> in, std::shared_ptr<std::ostream> out) : OMCodingBase(in, out)
    {
    }

    void write(openminecraft::network::OMNetworkPacket) override;
    auto read() -> openminecraft::network::OMNetworkPacket override;

    void switchState(OMProtocolMinecraftState s)
    {
        state = s;
    }
    auto queryState() -> OMProtocolMinecraftState
    {
        return state;
    }
};

class OMProtocolMinecraftTransformer
{
  protected:
    OMProtocolMinecraftHandler &handler;
    OMProtocolMinecraft &protocol;

  public:
    OMProtocolMinecraftTransformer(OMProtocolMinecraft &protocol, OMProtocolMinecraftHandler &hnd)
        : protocol(protocol), handler(hnd)
    {
    }
    ~OMProtocolMinecraftTransformer() = default;

    virtual void sendHandshake(std::string host, uint16_t port, int nextstate) = 0;
    virtual void sendStatusRequest() = 0;
    virtual void sendStatusPingRequest(int64_t tick) = 0;

    virtual void sendLoginHello(std::string name, openminecraft::util::OMUuid uuid) = 0;

    virtual void handle() = 0;
};
} // namespace openminecraftshell::network

#endif