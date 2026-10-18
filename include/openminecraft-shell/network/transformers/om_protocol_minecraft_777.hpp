#ifndef OM_PROTOCOL_MINECRAFT_777_HPP
#define OM_PROTOCOL_MINECRAFT_777_HPP

#include "openminecraft-shell/network/om_data_component_mapping.hpp"
#include "openminecraft-shell/network/transformers/om_protocol_minecraft_general.hpp"
namespace openminecraft::shell::network::transformers
{
class OMProtocolMinecraftTransformer777 : public OMProtocolMinecraftTransformerGeneral
{
  public:
    OMProtocolMinecraftTransformer777(OMProtocolMinecraft &p, OMProtocolMinecraftHandler &hnd)
        : OMProtocolMinecraftTransformerGeneral(p, hnd)
    {
        componentMapping.setNames(componentNames());
    }

    static auto componentNames() -> std::vector<std::string>;

    void sendLoginHello(std::string name, openminecraft::util::OMUuid uuid) override;
    void sendLoginAcknowledged() override;
    void sendConfigurationKnownPacks(std::vector<OMProtocolKnownPack>) override;
    void sendConfigurationFinishAcknowledged() override;
    void sendPlayConfirmTeleportation(uint32_t teleportID, double x, double y, double z, float yaw,
                                      float pitch) override;
    void sendPlayKeepAlive(int64_t id) override;

    void handleRest(OMProtocolMinecraftState, openminecraft::network::OMNetworkPacket &) override;

  private:
    openminecraft::shell::network::OMDataComponentMapping componentMapping;
    openminecraft::log::OMLogger logger = {"OMProtocolMinecraftTransformer777", this};
};
} // namespace openminecraft::shell::network::transformers

#endif