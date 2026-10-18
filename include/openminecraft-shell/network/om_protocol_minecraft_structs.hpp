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
// INFO: component *kinds* in the order of net.minecraft.core.component.DataComponents; the wire ids are
// resolved through OMDataComponentMapping (installed by the transformer per protocol version).
enum class OMDataComponentType : int32_t
{
    CustomData = 0,   // minecraft:custom_data
    MaxStackSize = 1,   // minecraft:max_stack_size
    MaxDamage = 2,   // minecraft:max_damage
    Damage = 3,   // minecraft:damage
    Unbreakable = 4,   // minecraft:unbreakable
    UseEffects = 5,   // minecraft:use_effects
    CustomName = 6,   // minecraft:custom_name
    MinimumAttackCharge = 7,   // minecraft:minimum_attack_charge
    DamageType = 8,   // minecraft:damage_type
    ItemName = 9,   // minecraft:item_name
    ItemModel = 10,   // minecraft:item_model
    Lore = 11,   // minecraft:lore
    Rarity = 12,   // minecraft:rarity
    Enchantments = 13,   // minecraft:enchantments
    CanPlaceOn = 14,   // minecraft:can_place_on
    CanBreak = 15,   // minecraft:can_break
    AttributeModifiers = 16,   // minecraft:attribute_modifiers
    CustomModelData = 17,   // minecraft:custom_model_data
    TooltipDisplay = 18,   // minecraft:tooltip_display
    RepairCost = 19,   // minecraft:repair_cost
    CreativeSlotLock = 20,   // minecraft:creative_slot_lock
    EnchantmentGlintOverride = 21,   // minecraft:enchantment_glint_override
    IntangibleProjectile = 22,   // minecraft:intangible_projectile
    Food = 23,   // minecraft:food
    Consumable = 24,   // minecraft:consumable
    UseRemainder = 25,   // minecraft:use_remainder
    UseCooldown = 26,   // minecraft:use_cooldown
    DamageResistant = 27,   // minecraft:damage_resistant
    Tool = 28,   // minecraft:tool
    Weapon = 29,   // minecraft:weapon
    AttackRange = 30,   // minecraft:attack_range
    Enchantable = 31,   // minecraft:enchantable
    Equippable = 32,   // minecraft:equippable
    Repairable = 33,   // minecraft:repairable
    Glider = 34,   // minecraft:glider
    TooltipStyle = 35,   // minecraft:tooltip_style
    DeathProtection = 36,   // minecraft:death_protection
    BlocksAttacks = 37,   // minecraft:blocks_attacks
    PiercingWeapon = 38,   // minecraft:piercing_weapon
    KineticWeapon = 39,   // minecraft:kinetic_weapon
    AttackAnimation = 40,   // minecraft:attack_animation
    InteractAnimation = 41,   // minecraft:interact_animation
    AdditionalTradeCost = 42,   // minecraft:additional_trade_cost
    BlockTransformer = 43,   // minecraft:block_transformer
    VillagerFood = 44,   // minecraft:villager_food
    StoredEnchantments = 45,   // minecraft:stored_enchantments
    Dye = 46,   // minecraft:dye
    DyedColor = 47,   // minecraft:dyed_color
    MapId = 48,   // minecraft:map_id
    MapDecorations = 49,   // minecraft:map_decorations
    MapPostProcessing = 50,   // minecraft:map_post_processing
    ChargedProjectiles = 51,   // minecraft:charged_projectiles
    BundleContents = 52,   // minecraft:bundle_contents
    PotionContents = 53,   // minecraft:potion_contents
    PotionDurationScale = 54,   // minecraft:potion_duration_scale
    SuspiciousStewEffects = 55,   // minecraft:suspicious_stew_effects
    WritableBookContent = 56,   // minecraft:writable_book_content
    WrittenBookContent = 57,   // minecraft:written_book_content
    Trim = 58,   // minecraft:trim
    DebugStickState = 59,   // minecraft:debug_stick_state
    EntityData = 60,   // minecraft:entity_data
    BucketEntityData = 61,   // minecraft:bucket_entity_data
    BlockEntityData = 62,   // minecraft:block_entity_data
    Instrument = 63,   // minecraft:instrument
    ProvidesTrimMaterial = 64,   // minecraft:provides_trim_material
    OminousBottleAmplifier = 65,   // minecraft:ominous_bottle_amplifier
    JukeboxPlayable = 66,   // minecraft:jukebox_playable
    ProvidesBannerPatterns = 67,   // minecraft:provides_banner_patterns
    Recipes = 68,   // minecraft:recipes
    LodestoneTracker = 69,   // minecraft:lodestone_tracker
    FireworkExplosion = 70,   // minecraft:firework_explosion
    Fireworks = 71,   // minecraft:fireworks
    Profile = 72,   // minecraft:profile
    NoteBlockSound = 73,   // minecraft:note_block_sound
    BannerPatterns = 74,   // minecraft:banner_patterns
    BaseColor = 75,   // minecraft:base_color
    PotDecorations = 76,   // minecraft:pot_decorations
    Container = 77,   // minecraft:container
    BlockState = 78,   // minecraft:block_state
    Bees = 79,   // minecraft:bees
    SulfurCubeContent = 80,   // minecraft:sulfur_cube_content
    Lock = 81,   // minecraft:lock
    ContainerLoot = 82,   // minecraft:container_loot
    BreakSound = 83,   // minecraft:break_sound
    Compostable = 84,   // minecraft:compostable
    CookingFuel = 85,   // minecraft:cooking_fuel
    BrewingFuel = 86,   // minecraft:brewing_fuel
    MobVisibility = 87,   // minecraft:mob_visibility
    VillagerVariant = 88,   // minecraft:villager/variant
    WolfVariant = 89,   // minecraft:wolf/variant
    WolfSoundVariant = 90,   // minecraft:wolf/sound_variant
    WolfCollar = 91,   // minecraft:wolf/collar
    FoxVariant = 92,   // minecraft:fox/variant
    SalmonSize = 93,   // minecraft:salmon/size
    ParrotVariant = 94,   // minecraft:parrot/variant
    TropicalFishPattern = 95,   // minecraft:tropical_fish/pattern
    TropicalFishBaseColor = 96,   // minecraft:tropical_fish/base_color
    TropicalFishPatternColor = 97,   // minecraft:tropical_fish/pattern_color
    MooshroomVariant = 98,   // minecraft:mooshroom/variant
    RabbitVariant = 99,   // minecraft:rabbit/variant
    PigVariant = 100,   // minecraft:pig/variant
    PigSoundVariant = 101,   // minecraft:pig/sound_variant
    CowVariant = 102,   // minecraft:cow/variant
    CowSoundVariant = 103,   // minecraft:cow/sound_variant
    ChickenVariant = 104,   // minecraft:chicken/variant
    ChickenSoundVariant = 105,   // minecraft:chicken/sound_variant
    ZombieNautilusVariant = 106,   // minecraft:zombie_nautilus/variant
    FrogVariant = 107,   // minecraft:frog/variant
    HorseVariant = 108,   // minecraft:horse/variant
    PaintingVariant = 109,   // minecraft:painting/variant
    LlamaVariant = 110,   // minecraft:llama/variant
    AxolotlVariant = 111,   // minecraft:axolotl/variant
    CatVariant = 112,   // minecraft:cat/variant
    CatSoundVariant = 113,   // minecraft:cat/sound_variant
    CatCollar = 114,   // minecraft:cat/collar
    SheepColor = 115,   // minecraft:sheep/color
    ShulkerColor = 116,   // minecraft:shulker/color
    ProvidesPotteryPattern = 117,   // minecraft:provides_pottery_pattern
    SignTextFront = 118,   // minecraft:sign_text_front
    SignTextBack = 119,   // minecraft:sign_text_back
    Waxed = 120,   // minecraft:waxed
    CushionColor = 121,   // minecraft:cushion/color
};

// One entry of minecraft:attribute_modifiers.
struct OMProtocolItemAttributeModifier
{
    int32_t attributeId = 0;   // ID in the minecraft:attribute registry
    std::string id;            // Modifier ID (Identifier)
    double value = 0.0;        // modifier value
    int32_t operation = 0;     // 0 add, 1 multiply base, 2 multiply total
    int32_t slot = 0;          // 0 any, 1 main hand, ... 9 body
};

// Inline armor trim material: Suffix | Overrides | Overridden Asset Name | Description.
struct OMProtocolTrimMaterialOverride
{
    std::string armorMaterial; // Armor Material Type identifier
    std::string assetName;
};

struct OMProtocolTrimMaterial
{
    std::string suffix;
    std::vector<OMProtocolTrimMaterialOverride> overrides;
    std::string overriddenAssetName;
    std::shared_ptr<openminecraft::specs::nbt::OMNbtTag> description; // Text Component
};

// Inline armor trim pattern: Asset Name | Template Item | Description | Decal.
struct OMProtocolTrimPattern
{
    std::string assetName;
    int32_t templateItem = 0;
    std::shared_ptr<openminecraft::specs::nbt::OMNbtTag> description;
    bool decal = false;
};

// Inline sound event: Identifier + Optional Float range.
struct OMProtocolSoundEvent
{
    std::string location;
    std::optional<float> range;
};

// Inline jukebox song: Sound Event | Description | Duration | Output.
struct OMProtocolJukeboxSong
{
    openminecraft::network::OMIDOr<OMProtocolSoundEvent> soundEvent;
    std::shared_ptr<openminecraft::specs::nbt::OMNbtTag> description; // Text Component
    float duration = 0.0f;
    int32_t output = 0;
};

// One page of a writable book: raw text + optional chat-filtered text.
struct OMProtocolBookPage
{
    std::string raw;
    std::optional<std::string> filtered;
};

// One page of a written book: raw text component + optional filtered component.
struct OMProtocolWrittenBookPage
{
    std::shared_ptr<openminecraft::specs::nbt::OMNbtTag> raw;
    std::shared_ptr<openminecraft::specs::nbt::OMNbtTag> filtered;
};

struct OMProtocolPotionDetail;

// Potion effect detail; Hidden Effect is a prefixed optional Detail and therefore recursive.
struct OMProtocolPotionDetail
{
    int32_t amplifier = 0;
    int32_t duration = 0;     // -1 means infinite
    bool ambient = false;
    bool showParticles = true;
    bool showIcon = true;
    std::shared_ptr<OMProtocolPotionDetail> hiddenEffect; // recursive optional detail
};

// Potion effect: type id + detail.
struct OMProtocolPotionEffect
{
    int32_t typeId = 0; // ID in the minecraft:mob_effect registry
    OMProtocolPotionDetail detail;
};
// Firework explosion: shape | colors | fade colors | trail | twinkle.
struct OMProtocolFireworkExplosion
{
    int32_t shape = 0; // 0 small ball, 1 large ball, 2 star, 3 creeper, 4 burst
    std::vector<int32_t> colors;
    std::vector<int32_t> fadeColors;
    bool hasTrail = false;
    bool hasTwinkle = false;
};

// One rule of minecraft:tool.
struct OMProtocolToolRule
{
    openminecraft::network::OMIdSet blocks; // ID Set in the minecraft:block registry
    std::optional<float> speed;             // speed for this rule
    std::optional<bool> correctDrop;        // drop only when this is the correct tool
};

struct OMProtocolItemStack; // forward declaration: Slot payloads use shared_ptr to break the value type recursion

// One entry of a data component patch: registry id plus the payload of that component.
// INFO: the payload layout depends on the component type, so only known types are decoded;
// unknown ones keep their raw head bytes and set truncated.
struct OMProtocolDataComponent
{
    OMDataComponentType type = OMDataComponentType::CustomData;
    int32_t intValue = 0;               // VarInt / VarInt enum payload
    float floatValue = 0.0f;            // Float payload
    bool boolValue = false;             // Boolean payload
    std::string stringValue;            // Identifier / Text Component (raw JSON)
    std::optional<std::string> optionalString; // Optional Identifier payload (use_cooldown group)
    std::vector<int32_t> intListC;      // third int array (bees min ticks)
    std::vector<std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>> nbtList; // NBT array (bees entity data)
    std::optional<openminecraft::network::OMPackedPos> position; // lodestone_tracker position
    bool secondBool = false;            // second boolean (lodestone_tracker tracked)
    std::vector<int32_t> intList;       // prefixed array of VarInt (ids / levels / component ids)
    std::vector<std::string> stringList; // prefixed array of strings (lore lines)
    std::vector<float> floatList;        // prefixed array of floats (custom_model_data)
    std::vector<int32_t> intListB;       // secondary int array (levels / ids)
    std::vector<bool> boolList;          // prefixed array of booleans
    openminecraft::network::OMIdSet idSet; // ID Set payload (damage_resistant / repairable / banner patterns)
    std::shared_ptr<openminecraft::specs::nbt::OMNbtTag> nbt; // network NBT payload (custom_data / entity_data)
    std::vector<OMProtocolItemAttributeModifier> attributeModifiers; // minecraft:attribute_modifiers
    openminecraft::network::OMIDOr<OMProtocolTrimMaterial> trimMaterial; // minecraft:trim
    openminecraft::network::OMIDOr<OMProtocolTrimPattern> trimPattern;   // minecraft:trim
    openminecraft::network::OMIDOr<OMProtocolJukeboxSong> jukeboxSong; // minecraft:jukebox_playable
    openminecraft::network::OMIDOr<OMProtocolTrimMaterial> providesTrimMaterial; // minecraft:provides_trim_material
    std::vector<OMProtocolBookPage> bookPages;              // writable_book_content
    std::vector<OMProtocolWrittenBookPage> writtenBookPages; // written_book_content
    openminecraft::network::OMIDOr<OMProtocolSoundEvent> soundEvent; // consumable sound
    std::vector<OMProtocolPotionEffect> potionEffects; // potion_contents / apply_effects
    std::optional<int32_t> potionId;                   // potion_contents potion id
    std::optional<int32_t> potionColor;                // potion_contents custom color
    OMProtocolFireworkExplosion fireworkExplosion;          // firework_explosion
    std::vector<OMProtocolFireworkExplosion> fireworkExplosions; // fireworks
    std::vector<int32_t> componentTypes; // exact component matcher types of a block predicate
    std::vector<OMProtocolToolRule> toolRules; // rule list of minecraft:tool
    std::shared_ptr<OMProtocolItemStack> stack;               // Slot payload (use_remainder / sulfur_cube_content)
    std::vector<std::shared_ptr<OMProtocolItemStack>> stacks; // Slot array (charged_projectiles / bundle_contents / container)
    bool truncated = false;             // payload not decodable yet
    std::vector<uint8_t> head;          // raw payload bytes when truncated
};

// DataComponentPatch wire order: added count | removed count | added payloads | removed ids.
struct OMProtocolDataComponentPatch
{
    std::vector<OMProtocolDataComponent> added;
    std::vector<int32_t> removed;
    bool truncated = false; // set when any added component could not be delimited
};

struct OMProtocolItemStack
{
    int32_t count = 0;
    int32_t itemId = 0;
    OMProtocolDataComponentPatch components;
};

// Slot Display: how an item should be rendered (recipe book, advises, etc.).
// Wire order: type (VarInt, minecraft:slot_display registry id) followed by type-specific data.
// INFO: the slot_display registry is data driven; the ids below follow the vanilla registration order.
enum class OMSlotDisplayKind : int32_t
{
    Empty = 0,
    AnyFuel = 1,
    WithAnyPotion = 2,
    OnlyWithComponent = 3,
    Item = 4,
    ItemStack = 5,
    Tag = 6,
    Dyed = 7,
    SmithingTrim = 8,
    WithRemainder = 9,
    Composite = 10
};

struct OMSlotDisplay
{
    int32_t type = 0;
    OMSlotDisplayKind kind = OMSlotDisplayKind::Empty;
    std::vector<OMSlotDisplay> nested; // base / dye+target / material / ingredient+remainder / options
    int32_t itemId = 0;                // minecraft:item
    int32_t componentTypeId = 0;       // minecraft:only_with_component
    int32_t patternId = 0;             // minecraft:smithing_trim
    std::string tag;                   // minecraft:tag (identifier, no leading '#')
    OMProtocolItemStack stack;         // minecraft:item_stack
    bool truncated = false;            // item stack with added components cannot be delimited yet
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

// Disconnect (0x20): the server kicks the client with a text component reason.
// Wire order: reason (Text Component, network NBT with an unnamed root tag)
struct OMProtocolDisconnect
{
    std::shared_ptr<openminecraft::specs::nbt::OMNbtTag> reason;
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
