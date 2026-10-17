#ifndef OM_PROTOCOL_MINECRAFT_HPP
#define OM_PROTOCOL_MINECRAFT_HPP

#include "openminecraft/network/om_network_packet.hpp"
#include "openminecraft/network/protocol/coding/om_coding.hpp"
#include "openminecraft/specs/nbt/om_nbt.hpp"
#include "openminecraft/specs/nbt/om_nbt_reader.hpp"
#include "openminecraft/util/om_util_uuid.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>
namespace openminecraft::shell::network
{
struct OMProtocolGameProfileProperty
{
    std::string name;
    std::string value;
    std::string signature;
};

struct OMProtocolKnownPack
{
    std::string namesp;
    std::string id;
    std::string version;
};

struct OMProtocolGameProfile
{
    std::string name;
    openminecraft::util::OMUuid uuid;
    std::vector<OMProtocolGameProfileProperty> properties;
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
    std::function<void(OMProtocolGameProfile)> handleLoginSuccess = [](OMProtocolGameProfile) {};
    std::function<void()> handleEnterConfiguration = []() {};
    std::function<void(std::string, openminecraft::network::OMNetworkPacket &)> handleConfigurationPluginMessage =
        [](std::string, openminecraft::network::OMNetworkPacket &) {};
    std::function<void(std::vector<std::string>)> handleConfigurationFeatureFlags = [](std::vector<std::string>) {};
    std::function<void(std::vector<std::tuple<std::string, std::string, std::string>>)>
        handleConfigurationReportDetails = [](std::vector<std::tuple<std::string, std::string, std::string>>) {};
    std::function<void(std::string,
                       const std::unordered_map<std::string, std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>> &)>
        handleConfigurationRegistryData =
            [](std::string,
               const std::unordered_map<std::string, std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>> &) {};
    std::function<void(std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> &)>
        handleConfigurationUpdateTag =
            [](std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> &) {};
    std::function<void()> handleConfigurationFinish = []() {};
    std::function<void()> handleEnterPlay = []() {};
    std::function<void(int, bool, std::vector<std::string>, uint32_t, uint32_t, uint32_t, bool, bool, bool, uint32_t,
                       std::string, int64_t, uint32_t, uint32_t, bool, bool, bool, std::string, int64_t, uint32_t,
                       uint32_t, bool, bool)>
        handlePlayLogin = [](int, bool, std::vector<std::string>, uint32_t, uint32_t, uint32_t, bool, bool, bool,
                             uint32_t, std::string, int64_t, uint32_t, uint32_t, bool, bool, bool, std::string, int64_t,
                             uint32_t, uint32_t, bool, bool) {};
    std::function<void(uint8_t, bool)> handlePlayChangeDifficulty = [](uint8_t, bool) {};
    std::function<void(uint8_t, float, float)> handlePlayPlayerAbilities = [](uint8_t, float, float) {};
    std::function<void(uint32_t)> handlePlaySetHeldSlot = [](uint32_t) {};
    std::function<void(int, uint8_t)> handlePlayEntityEvent = [](int, uint8_t) {};
};

class OMProtocolMinecraft : public openminecraft::network::protocol::OMCodingBase
{
  private:
    OMProtocolMinecraftState state = Handshake;
    bool compression = false;
    uint32_t compressionThreshold = 0;

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
        compressionThreshold = t;
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

    virtual void sendConfigurationKnownPacks(std::vector<OMProtocolKnownPack>) = 0;
    virtual void sendConfigurationFinishAcknowledged() = 0;

    virtual void handle() = 0;
};
} // namespace openminecraft::shell::network

template <> struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolKnownPack>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck) -> openminecraft::shell::network::OMProtocolKnownPack
    {
        return {pck.read<openminecraft::network::OMUtf8String>(), pck.read<openminecraft::network::OMUtf8String>(),
                pck.read<openminecraft::network::OMUtf8String>()};
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolKnownPack &value) -> void
    {
        pck.write(openminecraft::network::OMUtf8String(value.namesp));
        pck.write(openminecraft::network::OMUtf8String(value.id));
        pck.write(openminecraft::network::OMUtf8String(value.version));
    }
};

template <> struct openminecraft::network::OMNetworkPacketCodec<std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>
    {
        openminecraft::specs::nbt::OMNbtReader reader(pck.asStream());

        return reader.parse(false);
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolGameProfileProperty>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolGameProfileProperty
    {
        return {pck.read<openminecraft::network::OMUtf8String>(), pck.read<openminecraft::network::OMUtf8String>(),
                pck.read<openminecraft::network::OMUtf8String>()};
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolGameProfileProperty &value) -> void
    {
        pck.write(openminecraft::network::OMUtf8String(value.name));
        pck.write(openminecraft::network::OMUtf8String(value.value));
        pck.write(openminecraft::network::OMUtf8String(value.signature));
    }
};

#endif