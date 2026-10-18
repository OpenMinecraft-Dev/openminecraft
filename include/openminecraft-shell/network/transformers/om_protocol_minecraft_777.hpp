#ifndef OM_PROTOCOL_MINECRAFT_777_HPP
#define OM_PROTOCOL_MINECRAFT_777_HPP

#include "openminecraft-shell/network/transformers/om_protocol_minecraft_general.hpp"
namespace openminecraft::shell::network::transformers
{
class OMProtocolMinecraftTransformer777 : public OMProtocolMinecraftTransformerGeneral
{
  public:
    OMProtocolMinecraftTransformer777(OMProtocolMinecraft &p, OMProtocolMinecraftHandler &hnd)
        : OMProtocolMinecraftTransformerGeneral(p, hnd)
    {
    }

    void sendLoginHello(std::string name, openminecraft::util::OMUuid uuid) override;
    void sendLoginAcknowledged() override;
    void sendConfigurationKnownPacks(std::vector<OMProtocolKnownPack>) override;
    void sendConfigurationFinishAcknowledged() override;
    void sendPlayConfirmTeleportation(uint32_t teleportID, double x, double y, double z, float yaw,
                                      float pitch) override;

    void handleRest(OMProtocolMinecraftState, openminecraft::network::OMNetworkPacket &) override;

  private:
    openminecraft::log::OMLogger logger = {"OMProtocolMinecraftTransformer777", this};
};
} // namespace openminecraft::shell::network::transformers

#endif