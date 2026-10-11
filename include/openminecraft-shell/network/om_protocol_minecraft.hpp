#ifndef OM_PROTOCOL_MINECRAFT_HPP
#define OM_PROTOCOL_MINECRAFT_HPP

#include "openminecraft/network/om_network_packet.hpp"
#include "openminecraft/network/protocol/coding/om_coding.hpp"
#include "openminecraft/util/om_util_uuid.hpp"
#include <functional>
#include <memory>
#include <tuple>
#include <vector>
namespace openminecraftshell::network
{
struct OMProtocolGameProfile
{
    std::string name;
    openminecraft::util::OMUuid uuid;
    std::vector<std::tuple<std::string, std::string, std::string>> properties;
    openminecraft::util::OMUuid sessionId;
};

enum OMProtocolMinecraftState
{
    Handshake,
    Status,
    Login,
    Configuration,
    Play
};
class OMProtocolMinecraftHandler
{
  public:
    OMProtocolMinecraftHandler() = default;

    std::function<void(std::string)> handleStatusResponse = [](std::string) {};
    std::function<void(int64_t)> handlePongResponse = [](int64_t) {};
    std::function<void(OMProtocolGameProfile)> handleLoginSucccess = [](OMProtocolGameProfile) {};
    std::function<void()> handleEnterConfiguration = []() {};
    std::function<void(std::string, openminecraft::network::OMNetworkPacket &)> handleConfigurationPluginMessage =
        [](std::string, openminecraft::network::OMNetworkPacket &) {};
    std::function<void(std::vector<std::string>)> handleConfigurationFeatureFlags = [](std::vector<std::string>) {};
};

class OMProtocolMinecraft : public openminecraft::network::protocol::OMCodingBase
{
  private:
    OMProtocolMinecraftState state = Handshake;
    bool compression = false;
    uint32_t compressionThreshould = 0;

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

    void enableCompression(uint32_t t)
    {
        compression = true;
        compressionThreshould = t;
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
    virtual void sendLoginAcknowledged() = 0;

    virtual void sendConfigurationKnownPacks(std::vector<std::tuple<std::string, std::string, std::string>>) = 0;

    virtual void handle() = 0;
};
} // namespace openminecraftshell::network

#endif