#ifndef OM_PROTOCOL_MINECRAFT_HPP
#define OM_PROTOCOL_MINECRAFT_HPP

#include "openminecraft-shell/network/om_protocol_codecs.hpp"
#include "openminecraft-shell/network/om_protocol_minecraft_structs.hpp"
#include "openminecraft/network/om_network_packet.hpp"
#include "openminecraft/network/protocol/coding/om_coding.hpp"
#include "openminecraft/specs/nbt/om_nbt.hpp"
#include "openminecraft/specs/nbt/om_nbt_reader.hpp"
#include "openminecraft/util/om_util_uuid.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
namespace openminecraft::shell::network
{
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
    virtual ~OMProtocolMinecraftHandler() = default;

    virtual void handleStatusResponse(std::string statusJson) = 0;
    virtual void handleStatusPongResponse(int64_t clientTimestamp) = 0;

    virtual void handleLoginSuccess(OMProtocolGameProfile profile)
    {
    }

    virtual void handleEnterConfiguration()
    {
    }
    virtual void handleConfigurationPluginMessage(std::string channel, openminecraft::network::OMNetworkPacket &data)
    {
    }
    virtual void handleConfigurationFeatureFlags(std::vector<std::string> featureFlags)
    {
    }
    virtual void handleConfigurationReportDetails(const OMProtocolCustomReportDetails &details)
    {
    }
    virtual void handleConfigurationRegistryData(const OMProtocolRegistryData &data)
    {
    }
    virtual void handleConfigurationUpdateTag(const OMProtocolUpdateTags &tags)
    {
    }
    virtual void handleConfigurationFinish()
    {
    }

    virtual void handleEnterPlay()
    {
    }
    virtual void handlePlayLogin(int entityId, bool isHardcore, std::vector<std::string> dimensionNames,
                                 uint32_t maxPlayers, uint32_t viewDistance, uint32_t simulationDistance,
                                 bool reducedDebugInfo, bool enableRespawnScreen, bool doLimitedCrafting,
                                 uint32_t dimensionTypeId, std::string dimensionName, int64_t hashedSeed,
                                 uint32_t gameMode, uint32_t previousGameMode, bool isDebug, bool isFlat,
                                 bool hasDeathLocation, std::string deathDimensionName,
                                 openminecraft::network::OMPackedPos deathLocation, uint32_t portalCooldown,
                                 uint32_t seaLevel, bool onlineMode, bool secureChat)
    {
    }
    virtual void handlePlayChunkDataAndLight(const OMProtocolChunkDataAndLight &chunk)
    {
    }
    virtual void handlePlayUpdateEntityPosition(const OMProtocolUpdateEntityPosition &position)
    {
    }
    virtual void handlePlayUpdateEntityPositionRotation(const OMProtocolUpdateEntityPositionRotation &position)
    {
    }
    virtual void handlePlayTeleportEntity(const OMProtocolTeleportEntity &teleport)
    {
    }
    virtual void handlePlaySetExperience(const OMProtocolSetExperience &experience)
    {
    }
    virtual void handlePlaySetHealth(const OMProtocolSetHealth &health)
    {
    }
    virtual void handlePlayUpdateAdvancements(const OMProtocolUpdateAdvancements &advancements)
    {
    }
    virtual void handlePlayUpdateAttributes(const OMProtocolUpdateAttributes &attributes)
    {
    }
    virtual void handlePlaySetContainerContent(const OMProtocolSetContainerContent &content)
    {
    }
    virtual void handlePlayPostEffects(const OMProtocolPostEffects &effects)
    {
    }
    virtual void handlePlayPlayerInfoUpdate(const OMProtocolPlayerInfoUpdate &update)
    {
    }
    virtual void handlePlayServerData(const OMProtocolServerData &data)
    {
    }
    virtual void handlePlaySetDefaultSpawnPosition(const OMProtocolSetDefaultSpawnPosition &spawn)
    {
    }
    virtual void handlePlaySetTime(const OMProtocolSetTime &time)
    {
    }
    virtual void handlePlayInitializeBorder(double x, double z, double oldDiameter, double newDiameter,
                                            int64_t lerpTime, int32_t portalTeleportBoundary, int32_t warningBlocks,
                                            int32_t warningTime)
    {
    }
    virtual void handlePlayChangeDifficulty(uint8_t difficulty, bool difficultyLocked)
    {
    }
    virtual void handlePlayPlayerAbilities(uint8_t flags, float flyingSpeed, float fieldOfViewModifier)
    {
    }
    virtual void handlePlaySetHeldSlot(uint32_t slot)
    {
    }
    virtual void handlePlayEntityEvent(int entityId, uint8_t status)
    {
    }
    virtual void handlePlayGameEvent(int entityId, uint8_t status)
    {
    }
    virtual void handlePlayRecipeBookSettings(bool craftingRecipeBookOpen, bool craftingRecipeBookFilterActive,
                                              bool smeltingRecipeBookOpen, bool smeltingRecipeBookFilterActive,
                                              bool blastFurnaceRecipeBookOpen, bool blastFurnaceRecipeBookFilterActive,
                                              bool smokerRecipeBookOpen, bool smokerRecipeBookFilterActive)
    {
    }
    virtual void handlePlayPlayerPosition(uint32_t teleportId, double x, double y, double z, double velocityX,
                                          double velocityY, double velocityZ, float yaw, float pitch, int flags)
    {
    }
    virtual void handlePlayTickingState(float rate, bool isFrozen)
    {
    }
    virtual void handlePlayStepTick(uint32_t ticks)
    {
    }
    virtual void handlePlaySetChunkCacheCenter(int32_t centerX, int32_t centerZ)
    {
    }
    virtual void handlePlayChunkBatchStart()
    {
    }
    virtual void handlePlayChunkBatchEnd(uint32_t numOfChunks)
    {
    }
    virtual void handlePlaySetEntityVelocity(uint32_t entityId, double vx, double vy, double vz)
    {
    }
    virtual void handlePlayBlockUpdate(const OMProtocolBlockUpdate &update)
    {
    }
    virtual void handlePlayWorldEvent(const OMProtocolWorldEvent &event)
    {
    }
    virtual void handlePlayUpdateEntityRotation(const OMProtocolUpdateEntityRotation &rotation)
    {
    }
    virtual void handlePlaySetEquipment(const OMProtocolSetEquipment &equipment)
    {
    }
    virtual void handlePlayDamageEvent(const OMProtocolDamageEvent &event)
    {
    }
    virtual void handlePlayDisconnect(const OMProtocolDisconnect &disconnect)
    {
    }
    virtual void handlePlayUpdateLight(const OMProtocolUpdateLight &light)
    {
    }
    virtual void handlePlaySpawnEntity(const OMProtocolSpawnEntity &entity)
    {
    }
    virtual void handlePlayRemoveEntities(const OMProtocolRemoveEntities &entities)
    {
    }
    virtual void handlePlaySetHeadRotation(const OMProtocolSetHeadRotation &rotation)
    {
    }
    virtual void handlePlayKeepAlive(int64_t id)
    {
    }
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
    virtual void sendPlayConfirmTeleportation(uint32_t teleportID, double x, double y, double z, float yaw,
                                              float pitch) = 0;
    virtual void sendPlayKeepAlive(int64_t id) = 0;

    virtual void handle() = 0;
};
} // namespace openminecraft::shell::network

#endif
