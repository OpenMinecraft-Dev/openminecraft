#ifndef OM_PROTOCOL_MINECRAFT_STRUCTS_HPP
#define OM_PROTOCOL_MINECRAFT_STRUCTS_HPP

#include "openminecraft/network/om_network_packet.hpp"
#include "openminecraft/specs/nbt/om_nbt.hpp"
#include "openminecraft/util/om_util_uuid.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace openminecraft::shell::network
{
// One entry of the player's game profile (Login Success, 0x02).
// Wire order: name (String) | value (String) | signature (String).
// A String is a VarInt byte length followed by UTF-8 bytes; signature may be empty.
struct OMProtocolGameProfileProperty
{
    std::string name;
    std::string value;
    std::string signature;
};

// Full payload of Login Success (0x02).
// Wire order: uuid (UUID) | name (String) | properties (Array of OMProtocolGameProfileProperty) |
//             sessionId (UUID).
struct OMProtocolGameProfile
{
    openminecraft::util::OMUuid uuid;
    std::string name;
    std::vector<OMProtocolGameProfileProperty> properties;
    openminecraft::util::OMUuid sessionId;
};

// One property of a player profile inside Player Info Update (0x47, Add Player action).
// Wire order: name (String) | value (String) | isSigned (Bool) | signature (String, only when isSigned)
struct OMProtocolPlayerProperty
{
    std::string name;
    std::string value;
    bool isSigned = false;
    std::string signature;
};

// Chat session data of Player Info Update (0x47, Initialize Chat action).
// Wire order: sessionId (UUID) | publicKeyExpiry (Long) | publicKey (Byte Array) | keySignature (Byte Array)
struct OMProtocolChatSession
{
    openminecraft::util::OMUuid sessionId;
    int64_t publicKeyExpiry = 0;
    std::vector<uint8_t> publicKey;
    std::vector<uint8_t> keySignature;
};

// One player entry of Player Info Update (0x47). Which fields are filled depends on the action mask.
struct OMProtocolPlayerInfoEntry
{
    openminecraft::util::OMUuid uuid;
    std::optional<std::string> name;                                                 // 0x01 Add Player
    std::vector<OMProtocolPlayerProperty> properties;                                // 0x01 Add Player
    std::optional<OMProtocolChatSession> chat;                                       // 0x02 Initialize Chat
    std::optional<int32_t> gameMode;                                                 // 0x04 Update Game Mode
    std::optional<bool> listed;                                                      // 0x08 Update Listed
    std::optional<int32_t> latency;                                                  // 0x10 Update Latency
    std::optional<std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>> displayName; // 0x20 Update Display Name
    std::optional<bool> hat;                                                         // 0x40 Update Hat
    std::optional<int32_t> listOrder;                                                // 0x80 Update List Order
};

// Player Info Update (0x47): a batch of player entries, each carrying only the actions in the mask.
// Wire order: actions (Byte, bitset) | entries (Array of entry), each entry starts with the player UUID.
struct OMProtocolPlayerInfoUpdate
{
    uint8_t actions = 0;
    std::vector<OMProtocolPlayerInfoEntry> entries;
};

// One slot of Set Container Content (0x12). count 0 means an empty slot.
// Wire order: count (VarInt) | [itemId (VarInt) | added components (Array) | removed components (Array of VarInt)]
struct OMProtocolItemStack
{
    int32_t count = 0;
    int32_t itemId = 0;
    std::vector<int32_t> removedComponents;
};

// Set Container Content (0x12): the full content of a container window.
// Wire order: windowId (VarInt) | stateId (VarInt) | slots (Array of Item Stack) | carriedItem (Item Stack)
// TODO: truncated: data component not supported, add flag
struct OMProtocolSetContainerContent
{
    int32_t windowId = 0;
    int32_t stateId = 0;
    std::vector<OMProtocolItemStack> slots;
    OMProtocolItemStack carriedItem;
    bool truncated = false;
};

// One modifier of an attribute.
// Wire order: id (Identifier) | amount (Double) | operation (VarInt)
struct OMProtocolAttributeModifier
{
    std::string id;
    double amount = 0.0;
    int32_t operation = 0;
};

// One attribute snapshot of Update Attributes (0x86).
// Wire order: attributeId (VarInt registry id) | base (Double) | modifiers (Array of Modifier)
struct OMProtocolAttribute
{
    int32_t attributeId = 0;
    double base = 0.0;
    std::vector<OMProtocolAttributeModifier> modifiers;
};

// Update Attributes (0x86): every attribute of an entity.
// Wire order: entityId (VarInt) | attributes (Array of Attribute)
struct OMProtocolUpdateAttributes
{
    int32_t entityId = 0;
    std::vector<OMProtocolAttribute> attributes;
};

// One criterion of an advancement progress entry.
// Wire order: id (Identifier) | date (Prefixed Optional Long, present when achieved)
struct OMProtocolAdvancementCriterion
{
    std::string id;
    std::optional<int64_t> date;
};

// Progress of one advancement.
// Wire order: id (Identifier) | criteria (Array of Criterion)
struct OMProtocolAdvancementProgress
{
    std::string id;
    std::vector<OMProtocolAdvancementCriterion> criteria;
};

// One advancement of Update Advancements (0x85).
// Wire order: id (Identifier) | parent (Prefixed Optional Identifier) |
//             display (Prefixed Optional Display, not parsed yet) |
//             requirements (Array of Array of String) | sendsTelemetry (Bool) |
//             unknownTail (8 bytes)
struct OMProtocolAdvancement
{
    std::string id;
    std::optional<std::string> parent;
    std::vector<std::vector<std::string>> requirements;
    bool sendsTelemetry = false;
    int64_t unknownTail = 0;
};

// Update Advancements (0x85): the advancement tree and the player's progress.
// Wire order: reset (Bool) | advancement mapping (Array) | removed (Array of Identifier) |
//             progress mapping (Array) | showAdvancements (Bool)
struct OMProtocolUpdateAdvancements
{
    bool reset = false;
    std::vector<OMProtocolAdvancement> added;
    std::vector<std::string> removed;
    std::vector<OMProtocolAdvancementProgress> progress;
    bool showAdvancements = false;
    bool truncated = false;
};

// Set Health (0x6A): the player's health, food and saturation.
// Wire order: health (Float) | food (VarInt) | foodSaturation (Float)
struct OMProtocolSetHealth
{
    float health = 0.0f;
    int32_t food = 0;
    float foodSaturation = 0.0f;
};

// Set Experience (0x69): the player's experience bar, level and total experience.
// Wire order: experienceBar (Float, 0.0 - 1.0) | level (VarInt) | totalExperience (VarInt)
struct OMProtocolSetExperience
{
    float experienceBar = 0.0f;
    int32_t level = 0;
    int32_t totalExperience = 0;
};

// One heightmap of a chunk: a type id plus the packed 9-bit height values as longs.
struct OMProtocolHeightmap
{
    int32_t type = 0;
    std::vector<int64_t> values;
};

// A paletted container (block states or biomes) of one chunk section.
// bitsPerEntry 0 means a single value; 1-8 means an indirect palette; > 8 means direct registry ids.
struct OMProtocolPalettedContainer
{
    uint8_t bitsPerEntry = 0;
    std::vector<int32_t> palette;
    std::vector<int64_t> data;
};

// One 16x16x16 chunk section.
// Wire order: blockCount (Short) | fluidCount (Short) | blockStates (Paletted Container) |
//             biomes (Paletted Container)
struct OMProtocolChunkSection
{
    int16_t blockCount = 0;
    int16_t fluidCount = 0;
    OMProtocolPalettedContainer blockStates;
    OMProtocolPalettedContainer biomes;
};

// One block entity of a chunk.
// Wire order: packedXZ (Unsigned Byte) | y (Short) | type (VarInt) | data (NBT)
struct OMProtocolBlockEntity
{
    uint8_t packedXZ = 0;
    int16_t y = 0;
    int32_t type = 0;
    std::shared_ptr<openminecraft::specs::nbt::OMNbtTag> data;
};

// Light data of a chunk: four masks plus the light arrays themselves.
// Masks are java.util.BitSet.toByteArray() encoded: VarInt byte count + little-endian bytes;
// each light array is a VarInt 2048 followed by 2048 bytes of packed light values.
struct OMProtocolChunkLight
{
    openminecraft::network::OMBitSet skyLightMask;
    openminecraft::network::OMBitSet blockLightMask;
    openminecraft::network::OMBitSet emptySkyLightMask;
    openminecraft::network::OMBitSet emptyBlockLightMask;
    std::vector<std::vector<uint8_t>> skyLight;
    std::vector<std::vector<uint8_t>> blockLight;
};

// Chunk Data And Light (0x2E): a chunk's heightmaps, block data, block entities and light.
// Wire order: chunkX (Int) | chunkZ (Int) | heightmaps | data (Byte Array) |
//             blockEntities (Array) | light (4 masks + 2 arrays of byte arrays)
// In this protocol version heightmaps are not NBT: a VarInt entry count followed by, per entry,
// (VarInt type + VarInt long count + long array).
// A section header is two Shorts: non-air block count and fluid count. Since 1.21.5 the paletted
// container data array has no length prefix: its size is ceil(entries / (64 / bitsPerEntry)) and
// entries never span longs (padding is appended at the end).
// Verification: every section is decoded and its non-air block count must match blockCount.
struct OMProtocolChunkDataAndLight
{
    int32_t chunkX = 0;
    int32_t chunkZ = 0;
    std::vector<OMProtocolHeightmap> heightmaps;
    int32_t dataLength = 0;
    std::vector<OMProtocolChunkSection> sections;
    std::vector<OMProtocolBlockEntity> blockEntities;
    OMProtocolChunkLight light;
    int32_t declaredBlockCount = 0;
    int32_t decodedBlockCount = 0;
    bool valid = false;         // block data check: decoded non-air blocks == blockCount per section
    bool lightComplete = false; // whether the light data was fully parsed
    int32_t lightTrailingBytes = 0;
    std::vector<uint8_t> lightHead; // first bytes at the start of the light region
};
// One step of a stepped position path: absolute position plus its tick offset.
struct OMProtocolPositionStep
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    int32_t tickOffset = 0;
};

// Teleport Entity (0x23, called entity_position_sync since 1.21.2).
// Wire order: entityId (VarInt) | pathType (VarInt: 0 = linear, 1 = stepped) |
//             linear: x/y/z (Double) | stepped: steps (Array of Position Step) |
//             yaw (Float, degrees) | pitch (Float, degrees) | onGround (Bool)
struct OMProtocolTeleportEntity
{
    int32_t entityId = 0;
    int32_t pathType = 0;
    double endX = 0.0;
    double endY = 0.0;
    double endZ = 0.0;
    std::vector<OMProtocolPositionStep> steps;
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool onGround = false;
};

// Update Entity Position (0x36): a relative movement of an entity.
// Wire order: entityId (VarInt) | deltaX (Short) | deltaY (Short) | deltaZ (Short) | onGround (Bool)
// Deltas are fixed point with a scale of 1/4096 block; this struct exposes them in blocks.
struct OMProtocolUpdateEntityPosition
{
    static constexpr double deltaUnitsPerBlock = 4096.0;

    int32_t entityId = 0;
    double deltaX = 0.0;
    double deltaY = 0.0;
    double deltaZ = 0.0;
    bool onGround = false;
};

// Update Entity Position and Rotation (0x37): relative movement plus absolute rotation.
// Wire order: entityId (VarInt) | deltaX/Y/Z (Short, 1/4096 block) | yaw (Angle) | pitch (Angle) |
//             onGround (Bool)
struct OMProtocolUpdateEntityPositionRotation
{
    int32_t entityId = 0;
    double deltaX = 0.0;
    double deltaY = 0.0;
    double deltaZ = 0.0;
    openminecraft::network::OMAngle yaw;
    openminecraft::network::OMAngle pitch;
    bool onGround = false;
};

// Set Head Rotation (0x55): the absolute head yaw of an entity.
// Wire order: entityId (VarInt) | headYaw (Angle)
struct OMProtocolSetHeadRotation
{
    int32_t entityId = 0;
    openminecraft::network::OMAngle headYaw;
};

// Remove Entities (0x4E): the entity ids the client should drop from its world.
// Wire order: entityIds (Array of VarInt)
struct OMProtocolRemoveEntities
{
    std::vector<int32_t> entityIds;
};

// Spawn Entity (0x01): a new entity in the world.
// Wire order: entityId (VarInt) | uuid (UUID) | type (VarInt registry id) | x/y/z (Double) |
//             velocity (LpVec3) | pitch (Angle) | yaw (Angle) | headYaw (Angle) | data (VarInt)
struct OMProtocolSpawnEntity
{
    int32_t entityId = 0;
    openminecraft::util::OMUuid uuid;
    int32_t type = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    openminecraft::network::OMAngle pitch;
    openminecraft::network::OMAngle yaw;
    openminecraft::network::OMAngle headYaw;
    int32_t data = 0;
    openminecraft::network::OMLpVec3 velocity;
};

// Update Light (0x31): light data of a single chunk.
// Wire order: chunkX (VarInt) | chunkZ (VarInt) | light data (4 BitSets + sky/block light arrays)
struct OMProtocolUpdateLight
{
    int32_t chunkX = 0;
    int32_t chunkZ = 0;
    OMProtocolChunkLight light;
    bool complete = false;
};

// Post Effects (0x53): the post-processing effects the client should apply.
// Wire order: effects (Array of Identifier)
struct OMProtocolPostEffects
{
    std::vector<std::string> effects;
};

// Server Data (0x58): the MOTD and favicon a server advertises.
// Wire order: motd (anonymous NBT text component) | icon (Optional Byte Array: bool + VarInt length + PNG bytes)
struct OMProtocolServerData
{
    std::shared_ptr<openminecraft::specs::nbt::OMNbtTag> motd;
    std::optional<std::vector<uint8_t>> icon;
};

// Set Default Spawn Position (0x63): the world spawn point of a dimension.
// Wire order: dimension (Identifier) | position (Position: packed Long) | yaw (Float) | pitch (Float)
// A Position packs x (26 bits), z (26 bits) and y (12 bits) into one Long.
struct OMProtocolSetDefaultSpawnPosition
{
    std::string dimension;
    openminecraft::network::OMPackedPos position;
    float yaw = 0.0f;
    float pitch = 0.0f;
};

// Set Time (0x73): game time plus the state of every world clock.
// Wire order: gameTime (Long) | clocks (Array of Clock)
//   Clock: id (VarInt registry key) | totalTicks (VarLong) | partialTick (Float) | rate (Float)
struct OMProtocolSetTime
{
    struct Clock
    {
        int32_t id = 0;
        int64_t totalTicks = 0;
        float partialTick = 0.0f;
        float rate = 1.0f;
    };

    int64_t gameTime = 0;
    std::vector<Clock> clocks;
};

// One known resource pack exchanged during configuration (Select Known Packs).
// Wire order: namespace (String) | id (String) | version (String).
struct OMProtocolKnownPack
{
    std::string namesp;
    std::string id;
    std::string version;
};

// Registry Data (0x07): one registry with all of its entries, sent during configuration.
// Wire order: registry (String) | entries (Array of Entry).
// Entry: name (String) | data (Optional NBT: 1-byte presence flag + NBT payload).
struct OMProtocolRegistryData
{
    struct Entry
    {
        std::string name;
        std::optional<std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>> data;
    };

    std::string registry;
    std::vector<Entry> entries;
};

// Custom Report Details (0x0F): key/value metadata used when submitting chat reports.
// Wire order: details (Array of Detail).
// Detail: title (String) | description (String).
struct OMProtocolCustomReportDetails
{
    struct Detail
    {
        std::string title;
        std::string description;
    };

    std::vector<Detail> details;
};

// Update Tags (0x0E): registry tag groups applied during configuration.
// Wire order: groups (Array of TagGroup).
// TagGroup: registry (String) | tags (Array of Tag).
// Tag: name (String) | entries (Array of VarInt, the tagged entry ids).
struct OMProtocolUpdateTags
{
    struct Tag
    {
        std::string name;
        std::vector<openminecraft::network::OMVarInt> entries;
    };

    struct TagGroup
    {
        std::string registry;
        std::vector<Tag> tags;
    };

    std::vector<TagGroup> groups;
};
} // namespace openminecraft::shell::network

#endif
