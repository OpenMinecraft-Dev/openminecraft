#ifndef OM_PROTOCOL_MINECRAFT_GENERAL_HPP
#define OM_PROTOCOL_MINECRAFT_GENERAL_HPP

#include "openminecraft-shell/network/om_protocol_minecraft.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/network/om_network_packet.hpp"
namespace openminecraft::shell::network::transformers
{
class OMProtocolMinecraftTransformerGeneral : public OMProtocolMinecraftTransformer
{
  public:
    OMProtocolMinecraftTransformerGeneral(OMProtocolMinecraft &p, OMProtocolMinecraftHandler &hnd)
        : OMProtocolMinecraftTransformer(p, hnd)
    {
    }
    void sendHandshake(std::string host, uint16_t port, int nextstate) override;
    void sendStatusRequest() override;
    void sendStatusPingRequest(int64_t tick) override;

    void sendLoginHello(std::string name, openminecraft::util::OMUuid uuid) override = 0;
    void sendLoginAcknowledged() override = 0;
    void sendConfigurationKnownPacks(std::vector<std::tuple<std::string, std::string, std::string>>) override = 0;
    void sendConfigurationFinishAcknowledged() override = 0;

    void handle() override;
    virtual void handleRest(OMProtocolMinecraftState, openminecraft::network::OMNetworkPacket &);

  private:
    openminecraft::log::OMLogger logger = {"OMProtocolMinecraftTransformerGeneral", this};
};
} // namespace openminecraft::shell::network::transformers

#endif