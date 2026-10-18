//   FMT_INC=$(find "$HOME/.xmake/packages/f/fmt" -type d -name include | head -1)
//   c++ -std=c++17 -DFMT_HEADER_ONLY -I include -I "$FMT_INC" \
//       misc/om_network_packet_test.cpp -o /tmp/om_network_packet_test
//   /tmp/om_network_packet_test

#include "openminecraft-shell/network/om_protocol_minecraft.hpp"
#include "openminecraft/network/om_network_packet.hpp"

#include <cstdint>
#include <optional>
#include <cstdio>
#include <string>
#include <vector>

using namespace openminecraft::network;

// extension point demo: a custom type only needs an OMNetworkPacketCodec<T> specialization
struct TestPair
{
    int32_t left = 0;
    int32_t right = 0;
};

namespace openminecraft::network
{
template <> struct OMNetworkPacketCodec<TestPair>
{
    static constexpr bool supported = true;

    static auto read(OMNetworkPacket &pck) -> TestPair
    {
        return {pck.read<int32_t>(), pck.read<int32_t>()};
    }

    static auto write(OMNetworkPacket &pck, const TestPair &value) -> void
    {
        pck.write<int32_t>(value.left).write<int32_t>(value.right);
    }
};
} // namespace openminecraft::network

static int failures = 0;

#define CHECK(expr)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            std::fprintf(stderr, "FAILED: %s (%s:%d)\n", #expr, __FILE__, __LINE__);                                   \
            ++failures;                                                                                                \
        }                                                                                                              \
    } while (false)

static auto testPrefixArrays() -> void
{
    OMNetworkPacket p;

    // int32_t[3]
    p.write(OMVarInt(3));
    p.write<int32_t>(7);
    p.write<int32_t>(-1);
    p.write<int32_t>(1000);

    // int64_t[2]
    p.write(OMVarInt(2));
    p.write<int64_t>(0x1122334455667788LL);
    p.write<int64_t>(-2);

    // bool[3]
    p.write(OMVarInt(3));
    p.write<bool>(true);
    p.write<bool>(false);
    p.write<bool>(true);

    // float[2]
    p.write(OMVarInt(2));
    p.write<float>(1.5f);
    p.write<float>(-0.25f);

    // double[3]
    p.write(OMVarInt(3));
    p.write<double>(3.141592653589793);
    p.write<double>(-1e300);
    p.write<double>(0.0);

    // OMUtf8String[2]
    p.write(OMVarInt(2));
    p.write(OMUtf8String("minecraft:overworld"));
    p.write(OMUtf8String(""));

    // OMVarInt[3]
    p.write(OMVarInt(3));
    p.write(OMVarInt(0));
    p.write(OMVarInt(300));
    p.write(OMVarInt(-5));

    // std::string[2] (the natural plain-type form of OMUtf8String)
    p.write(OMVarInt(2));
    p.write(std::string("alpha"));
    p.write(std::string("beta"));

    // uint8_t[4]
    p.write(OMVarInt(4));
    p.write<uint8_t>(1);
    p.write<uint8_t>(2);
    p.write<uint8_t>(255);
    p.write<uint8_t>(0);

    // OMUuid[2]
    p.write(OMVarInt(2));
    p.write(openminecraft::util::OMUuid(0x0102030405060708LL, 0x1112131415161718LL));
    p.write(openminecraft::util::OMUuid(0LL, 0LL));

    p.reset();

    auto i32 = p.readPrefixArray<int32_t>();
    CHECK(i32.size() == 3 && i32[0] == 7 && i32[1] == -1 && i32[2] == 1000);

    auto i64 = p.readPrefixArray<int64_t>();
    CHECK(i64.size() == 2 && i64[0] == 0x1122334455667788LL && i64[1] == -2);

    auto b = p.readPrefixArray<bool>();
    CHECK(b.size() == 3 && b[0] && !b[1] && b[2]);

    auto f = p.readPrefixArray<float>();
    CHECK(f.size() == 2 && f[0] == 1.5f && f[1] == -0.25f);

    auto d = p.readPrefixArray<double>();
    CHECK(d.size() == 3 && d[0] == 3.141592653589793 && d[1] == -1e300 && d[2] == 0.0);

    auto s = p.readPrefixArray<OMUtf8String>();
    CHECK(s.size() == 2 && s[0].value == "minecraft:overworld" && s[1].value.empty());

    auto v = p.readPrefixArray<OMVarInt>();
    CHECK(v.size() == 3 && v[0].value == 0 && v[1].value == 300 && v[2].value == -5);

    auto ss = p.readPrefixArray<std::string>();
    CHECK(ss.size() == 2 && ss[0] == "alpha" && ss[1] == "beta");

    auto u8 = p.readPrefixArray<uint8_t>();
    CHECK(u8.size() == 4 && u8[0] == 1 && u8[1] == 2 && u8[2] == 255 && u8[3] == 0);

    auto ids = p.readPrefixArray<openminecraft::util::OMUuid>();
    CHECK(ids.size() == 2 && ids[0].lowBits() == 0x0102030405060708LL &&
          ids[0].highBits() == 0x1112131415161718LL && ids[1].lowBits() == 0LL && ids[1].highBits() == 0LL);
}

static auto testEmptyArray() -> void
{
    OMNetworkPacket p;
    p.write(OMVarInt(0));
    p.reset();

    auto empty = p.readPrefixArray<int32_t>();
    CHECK(empty.empty());
}

static auto testScalars() -> void
{
    OMNetworkPacket p;
    p.write(std::string("hello")).write(OMUtf8String("world"));
    p.write<float>(2.5f).write<double>(-7.25);
    p.write<bool>(true).write<uint8_t>(0x5a);
    p.write<int16_t>(-1234).write<int32_t>(-12345678).write<int64_t>(-1234567890123LL);
    p.reset();

    CHECK(p.read<std::string>() == "hello");
    CHECK(p.read<OMUtf8String>().value == "world");
    CHECK(p.read<float>() == 2.5f);
    CHECK(p.read<double>() == -7.25);
    CHECK(p.read<bool>());
    CHECK(p.read<uint8_t>() == 0x5a);
    CHECK(p.read<int16_t>() == -1234);
    CHECK(p.read<int32_t>() == -12345678);
    CHECK(p.read<int64_t>() == -1234567890123LL);
}

static auto testWireFormat() -> void
{
    // VarInt 300 -> AC 02；VarInt -5 -> FB FF FF FF 0F
    OMNetworkPacket p;
    p.write(OMVarInt(300)).write(OMVarInt(-5));

    const uint8_t expected[] = {0xAC, 0x02, 0xFB, 0xFF, 0xFF, 0xFF, 0x0F};
    const auto *actual = reinterpret_cast<const uint8_t *>(p.data());

    CHECK(p.datalen() == sizeof(expected));
    for (std::size_t i = 0; i < sizeof(expected); ++i)
    {
        CHECK(actual[i] == expected[i]);
    }
}

static auto testCustomCodec() -> void
{
    // scalars
    OMNetworkPacket p;
    p.write(TestPair{7, -9});
    p.reset();

    auto pair = p.read<TestPair>();
    CHECK(pair.left == 7 && pair.right == -9);

    // arrays: readPrefixArray reuses read<TestPair>() and works automatically
    OMNetworkPacket a;
    a.write(OMVarInt(2)).write(TestPair{1, 2}).write(TestPair{3, 4});
    a.reset();

    auto list = a.readPrefixArray<TestPair>();
    CHECK(list.size() == 2 && list[0].left == 1 && list[1].right == 4);
}

static auto testFixedAndPrefixArrayWrites() -> void
{
    // fixed arrays: no length prefix on the wire
    OMNetworkPacket fixed;
    fixed.writeFixedArray<int32_t>({1, -2, 3});
    fixed.writeFixedArray<uint8_t>({0x11, 0x22, 0x33});
    fixed.writeFixedArray<TestPair>({TestPair{1, 2}, TestPair{3, 4}});
    fixed.writeFixedArray<int32_t>({});

    fixed.reset();

    auto fi = fixed.readFixedArray<int32_t>(3);
    CHECK(fi.size() == 3 && fi[0] == 1 && fi[1] == -2 && fi[2] == 3);

    auto fb = fixed.readFixedArray<uint8_t>(3);
    CHECK(fb.size() == 3 && fb[0] == 0x11 && fb[1] == 0x22 && fb[2] == 0x33);

    auto fp = fixed.readFixedArray<TestPair>(2);
    CHECK(fp.size() == 2 && fp[0].left == 1 && fp[1].right == 4);

    auto fe = fixed.readFixedArray<int32_t>(0);
    CHECK(fe.empty());

    // prefix arrays: a VarInt element count comes first
    OMNetworkPacket prefixed;
    prefixed.writePrefixArray<int32_t>({1, -2, 3});
    prefixed.writePrefixArray<OMUtf8String>({OMUtf8String("aa"), OMUtf8String("bb")});
    prefixed.writePrefixArray<uint8_t>({0x11, 0x22});

    // the first wire byte must be the element count 3
    CHECK(reinterpret_cast<const uint8_t *>(prefixed.data())[0] == 0x03);

    prefixed.reset();

    auto pi = prefixed.readPrefixArray<int32_t>();
    CHECK(pi.size() == 3 && pi[1] == -2);

    auto ps = prefixed.readPrefixArray<OMUtf8String>();
    CHECK(ps.size() == 2 && ps[0].value == "aa" && ps[1].value == "bb");

    auto pb = prefixed.readPrefixArray<uint8_t>();
    CHECK(pb.size() == 2 && pb[1] == 0x22);
}

static auto testProtocolCodecs() -> void
{
    using openminecraft::shell::network::OMProtocolGameProfileProperty;
    using openminecraft::shell::network::OMProtocolKnownPack;

    OMNetworkPacket p;
    p.writePrefixArray<OMProtocolKnownPack>({{"minecraft", "core", "1.21"}, {"forge", "neoforge", "1"}});
    p.writePrefixArray<OMProtocolGameProfileProperty>({{"textures", "abc", "sig"}});
    p.reset();

    auto packs = p.readPrefixArray<OMProtocolKnownPack>();
    CHECK(packs.size() == 2);
    CHECK(packs[0].namesp == "minecraft" && packs[0].id == "core" && packs[0].version == "1.21");
    CHECK(packs[1].namesp == "forge" && packs[1].id == "neoforge");

    auto props = p.readPrefixArray<OMProtocolGameProfileProperty>();
    CHECK(props.size() == 1 && props[0].name == "textures" && props[0].signature == "sig");
}

static auto testGameProfileCodec() -> void
{
    using openminecraft::shell::network::OMProtocolGameProfile;

    OMProtocolGameProfile profile;
    profile.uuid = openminecraft::util::OMUuid(0x0102030405060708ULL, 0x1112131415161718ULL);
    profile.name = "Coder2_";
    profile.properties = {{"textures", "abc", "sig"}, {"other", "def", ""}};
    profile.sessionId = openminecraft::util::OMUuid(0x2122232425262728ULL, 0x3132333435363738ULL);

    OMNetworkPacket p;
    p.write(profile);
    p.reset();

    auto parsed = p.read<OMProtocolGameProfile>();
    CHECK(parsed.uuid.lowBits() == 0x0102030405060708ULL && parsed.uuid.highBits() == 0x1112131415161718ULL);
    CHECK(parsed.name == "Coder2_");
    CHECK(parsed.properties.size() == 2);
    CHECK(parsed.properties[0].name == "textures" && parsed.properties[0].signature == "sig");
    CHECK(parsed.properties[1].value == "def" && parsed.properties[1].signature.empty());
    CHECK(parsed.sessionId.lowBits() == 0x2122232425262728ULL && parsed.sessionId.highBits() == 0x3132333435363738ULL);
}

static auto testUpdateTagsCodec() -> void
{
    using openminecraft::shell::network::OMProtocolUpdateTags;

    OMProtocolUpdateTags tags;
    tags.groups = {
        {"minecraft:item", {{"minecraft:logs", {1, 2, 3}}, {"minecraft:planks", {}}}},
        {"minecraft:block", {{"minecraft:mineable/pickaxe", {42}}}},
    };

    OMNetworkPacket p;
    p.write(tags);
    p.reset();

    auto parsed = p.read<OMProtocolUpdateTags>();
    CHECK(parsed.groups.size() == 2);

    CHECK(parsed.groups[0].registry == "minecraft:item");
    CHECK(parsed.groups[0].tags.size() == 2);
    CHECK(parsed.groups[0].tags[0].name == "minecraft:logs");
    CHECK(parsed.groups[0].tags[0].entries.size() == 3);
    CHECK(parsed.groups[0].tags[0].entries[0].value == 1 && parsed.groups[0].tags[0].entries[2].value == 3);
    CHECK(parsed.groups[0].tags[1].entries.empty());

    CHECK(parsed.groups[1].registry == "minecraft:block");
    CHECK(parsed.groups[1].tags[0].name == "minecraft:mineable/pickaxe");
    CHECK(parsed.groups[1].tags[0].entries[0].value == 42);
}

static auto testVarLong() -> void
{
    OMNetworkPacket p;
    p.write(OMVarLong(0)).write(OMVarLong(1)).write(OMVarLong(300)).write(OMVarLong(-1));
    p.write(OMVarLong(0x7FFFFFFFFFFFFFFFLL)).write(OMVarLong(INT64_MIN));
    p.reset();

    CHECK(p.read<OMVarLong>().value == 0);
    CHECK(p.read<OMVarLong>().value == 1);
    CHECK(p.read<OMVarLong>().value == 300);
    CHECK(p.read<OMVarLong>().value == -1);
    CHECK(p.read<OMVarLong>().value == 0x7FFFFFFFFFFFFFFFLL);
    CHECK(p.read<OMVarLong>().value == INT64_MIN);

    // wire format: 300 -> AC 02; -1 -> FF x9 + 01 (12 bytes total)
    OMNetworkPacket w;
    w.write(OMVarLong(300)).write(OMVarLong(-1));

    const auto *bytes = reinterpret_cast<const uint8_t *>(w.data());
    CHECK(w.datalen() == 12);
    CHECK(bytes[0] == 0xAC && bytes[1] == 0x02);
    for (int i = 2; i < 11; ++i)
    {
        CHECK(bytes[i] == 0xFF);
    }
    CHECK(bytes[11] == 0x01);

    // arrays and implicit conversions
    OMNetworkPacket a;
    a.writePrefixArray<OMVarLong>({OMVarLong(-1), OMVarLong(1)});
    a.reset();

    auto list = a.readPrefixArray<OMVarLong>();
    CHECK(list.size() == 2 && list[0].value == -1 && list[1].value == 1);

    int64_t plain = OMVarLong(42);
    CHECK(plain == 42);
}

static auto testCustomReportDetailsCodec() -> void
{
    using openminecraft::shell::network::OMProtocolCustomReportDetails;

    OMProtocolCustomReportDetails details;
    details.details = {{"title-a", "desc-a"}, {"title-b", ""}};

    OMNetworkPacket p;
    p.write(details);
    p.reset();

    auto parsed = p.read<OMProtocolCustomReportDetails>();
    CHECK(parsed.details.size() == 2);
    CHECK(parsed.details[0].title == "title-a" && parsed.details[0].description == "desc-a");
    CHECK(parsed.details[1].title == "title-b" && parsed.details[1].description.empty());
}

static auto testSetTimeCodec() -> void
{
    using openminecraft::shell::network::OMProtocolSetTime;

    OMProtocolSetTime time;
    time.gameTime = 104334;
    time.clocks = {{0, 104334, 0.0f, 1.0f}, {1, 104000, 0.5f, 20.0f}};

    OMNetworkPacket p;
    p.write(time);
    p.reset();

    auto parsed = p.read<OMProtocolSetTime>();
    CHECK(parsed.gameTime == 104334);
    CHECK(parsed.clocks.size() == 2);
    CHECK(parsed.clocks[0].id == 0 && parsed.clocks[0].totalTicks == 104334);
    CHECK(parsed.clocks[0].partialTick == 0.0f && parsed.clocks[0].rate == 1.0f);
    CHECK(parsed.clocks[1].id == 1 && parsed.clocks[1].totalTicks == 104000);
    CHECK(parsed.clocks[1].partialTick == 0.5f && parsed.clocks[1].rate == 20.0f);
}

static auto testPackedPos() -> void
{
    auto pos = OMPackedPos::from(-496, 66, -224);
    CHECK(pos.x() == -496 && pos.y() == 66 && pos.z() == -224);

    auto maximum = OMPackedPos::from(33554431, 2047, 33554431);
    CHECK(maximum.x() == 33554431 && maximum.y() == 2047 && maximum.z() == 33554431);

    auto minimum = OMPackedPos::from(-33554432, -2048, -33554432);
    CHECK(minimum.x() == -33554432 && minimum.y() == -2048 && minimum.z() == -33554432);

    OMNetworkPacket p;
    p.write(pos);
    p.reset();

    auto parsed = p.read<OMPackedPos>();
    CHECK(parsed.value == pos.value);
    CHECK(parsed.x() == -496 && parsed.y() == 66 && parsed.z() == -224);
}

static auto testSpawnPositionCodec() -> void
{
    using openminecraft::shell::network::OMProtocolSetDefaultSpawnPosition;

    OMProtocolSetDefaultSpawnPosition spawn;
    spawn.dimension = "minecraft:overworld";
    spawn.position = OMPackedPos::from(-496, 66, -224);
    spawn.yaw = 0.0f;
    spawn.pitch = 90.0f;

    OMNetworkPacket p;
    p.write(spawn);
    p.reset();

    auto parsed = p.read<OMProtocolSetDefaultSpawnPosition>();
    CHECK(parsed.dimension == "minecraft:overworld");
    CHECK(parsed.position.x() == -496 && parsed.position.y() == 66 && parsed.position.z() == -224);
    CHECK(parsed.yaw == 0.0f && parsed.pitch == 90.0f);

    // edge values: both ends of the 26-bit and 12-bit two's complement ranges
    OMProtocolSetDefaultSpawnPosition edge;
    edge.dimension = "minecraft:the_nether";
    edge.position = OMPackedPos::from(33554431, -2048, -33554432);

    OMNetworkPacket q;
    q.write(edge);
    q.reset();

    auto edgeParsed = q.read<OMProtocolSetDefaultSpawnPosition>();
    CHECK(edgeParsed.position.x() == 33554431 && edgeParsed.position.y() == -2048 &&
          edgeParsed.position.z() == -33554432);
}

static auto testPostEffectsCodec() -> void
{
    using openminecraft::shell::network::OMProtocolPostEffects;

    OMProtocolPostEffects effects;
    effects.effects = {"minecraft:spider", "minecraft:invert"};

    OMNetworkPacket p;
    p.write(effects);
    p.reset();

    auto parsed = p.read<OMProtocolPostEffects>();
    CHECK(parsed.effects.size() == 2);
    CHECK(parsed.effects[0] == "minecraft:spider" && parsed.effects[1] == "minecraft:invert");

    OMProtocolPostEffects empty;
    OMNetworkPacket q;
    q.write(empty);
    q.reset();
    CHECK(q.read<OMProtocolPostEffects>().effects.empty());
}

static auto testContainerContentCodec() -> void
{
    using openminecraft::shell::network::OMProtocolSetContainerContent;

    OMNetworkPacket p;
    p.write(OMVarInt(0)).write(OMVarInt(1)).write(OMVarInt(2)); // window, state, 2 slots
    p.write(OMVarInt(0));                                       // empty slot
    // slot: count=3, itemId=5, addedCount=0, removedCount=1, removed id=7
    p.write(OMVarInt(3)).write(OMVarInt(5)).write(OMVarInt(0)).write(OMVarInt(1)).write(OMVarInt(7));
    p.write(OMVarInt(0)); // carried item: empty
    p.reset();

    auto parsed = p.read<OMProtocolSetContainerContent>();
    CHECK(parsed.windowId == 0 && parsed.stateId == 1 && parsed.slots.size() == 2);
    CHECK(parsed.slots[0].count == 0);
    CHECK(parsed.slots[1].count == 3 && parsed.slots[1].itemId == 5);
    CHECK(parsed.slots[1].components.removed.size() == 1 && parsed.slots[1].components.removed[0] == 7);
    CHECK(parsed.carriedItem.count == 0 && !parsed.truncated);
}

static auto testUpdateAttributesCodec() -> void
{
    using openminecraft::shell::network::OMProtocolUpdateAttributes;

    OMProtocolUpdateAttributes attributes;
    attributes.entityId = 6795;
    attributes.attributes = {
        {8, 4.5, {}},
        {26, 0.1, {{"minecraft:test_modifier", 0.5, 2}}},
    };

    OMNetworkPacket p;
    p.write(attributes);
    p.reset();

    auto parsed = p.read<OMProtocolUpdateAttributes>();
    CHECK(parsed.entityId == 6795 && parsed.attributes.size() == 2);
    CHECK(parsed.attributes[0].attributeId == 8 && parsed.attributes[0].base == 4.5);
    CHECK(parsed.attributes[0].modifiers.empty());
    CHECK(parsed.attributes[1].attributeId == 26 && parsed.attributes[1].base == 0.1);
    CHECK(parsed.attributes[1].modifiers.size() == 1);
    CHECK(parsed.attributes[1].modifiers[0].id == "minecraft:test_modifier");
    CHECK(parsed.attributes[1].modifiers[0].amount == 0.5 && parsed.attributes[1].modifiers[0].operation == 2);
}

static auto testUpdateAdvancementsCodec() -> void
{
    using openminecraft::shell::network::OMProtocolUpdateAdvancements;

    OMNetworkPacket p;
    p.write<bool>(true);                    // reset
    p.write(OMVarInt(1));                   // added count
    p.write(OMUtf8String("minecraft:test")); // id
    p.write<bool>(false);                   // parent absent
    p.write<bool>(false);                   // display absent
    p.write(OMVarInt(1));                   // requirement groups
    p.write(OMVarInt(1));                   // group size
    p.write(OMUtf8String("crit"));          // criterion
    p.write<bool>(true);                    // sends telemetry
    p.write<int64_t>(0);                    // unknown tail
    p.write(OMVarInt(0));                   // removed
    p.write(OMVarInt(1));                   // progress count
    p.write(OMUtf8String("minecraft:test")); // progress id
    p.write(OMVarInt(1));                   // criteria
    p.write(OMUtf8String("crit"));          // criterion id
    p.write<bool>(true);                    // achieved
    p.write<int64_t>(1700000000000LL);      // date
    p.write<bool>(true);                    // show advancements
    p.reset();

    auto parsed = p.read<OMProtocolUpdateAdvancements>();
    CHECK(parsed.reset && parsed.showAdvancements && !parsed.truncated);
    CHECK(parsed.added.size() == 1 && parsed.added[0].id == "minecraft:test");
    CHECK(parsed.added[0].requirements.size() == 1 && parsed.added[0].requirements[0][0] == "crit");
    CHECK(parsed.added[0].sendsTelemetry && parsed.added[0].unknownTail == 0);
    CHECK(parsed.progress.size() == 1 && parsed.progress[0].id == "minecraft:test");
    CHECK(parsed.progress[0].criteria.size() == 1 && parsed.progress[0].criteria[0].id == "crit");
    CHECK(parsed.progress[0].criteria[0].date == 1700000000000LL);
}

static auto testSetHealthCodec() -> void
{
    using openminecraft::shell::network::OMProtocolSetHealth;

    OMProtocolSetHealth health;
    health.health = 20.0f;
    health.food = 20;
    health.foodSaturation = 5.0f;

    OMNetworkPacket p;
    p.write(health);
    p.reset();

    auto parsed = p.read<OMProtocolSetHealth>();
    CHECK(parsed.health == 20.0f);
    CHECK(parsed.food == 20);
    CHECK(parsed.foodSaturation == 5.0f);
}

static auto testSetExperienceCodec() -> void
{
    using openminecraft::shell::network::OMProtocolSetExperience;

    OMProtocolSetExperience experience;
    experience.experienceBar = 0.75f;
    experience.level = 12;
    experience.totalExperience = 1395;

    OMNetworkPacket p;
    p.write(experience);
    p.reset();

    auto parsed = p.read<OMProtocolSetExperience>();
    CHECK(parsed.experienceBar == 0.75f);
    CHECK(parsed.level == 12);
    CHECK(parsed.totalExperience == 1395);
}

static auto testBitSet() -> void
{
    OMBitSet set;
    set.data = {0b1011ULL, 1ULL << 63};

    CHECK(set.size() == 128);
    CHECK(set.test(0) && set.test(1) && !set.test(2) && set.test(3));
    CHECK(!set.test(64) && set.test(127));
    CHECK(set.count() == 4);

    OMNetworkPacket p;
    p.write(set);
    p.reset();

    auto parsed = p.read<OMBitSet>();
    CHECK(parsed.data == set.data);
    CHECK(parsed.test(3) && parsed.test(127) && !parsed.test(2));

    OMBitSet empty;
    OMNetworkPacket q;
    q.write(empty);
    q.reset();
    CHECK(q.read<OMBitSet>().data.empty());
}

static auto testIDOr() -> void
{
    OMIDOr<OMUtf8String> inlineValue;
    inlineValue.id = 0;
    inlineValue.value = OMUtf8String("minecraft:test");

    OMIDOr<OMUtf8String> referenced;
    referenced.id = 3; // registry id 2 + 1

    OMNetworkPacket p;
    p.write(inlineValue);
    p.write(referenced);
    p.reset();

    auto a = p.read<OMIDOr<OMUtf8String>>();
    CHECK(a.id == 0 && a.isInline() && a.value && a.value->value == "minecraft:test");

    auto b = p.read<OMIDOr<OMUtf8String>>();
    CHECK(b.id == 3 && !b.isInline() && b.registryId() == 2 && !b.value);

    // plain scalar types work inline as well
    OMNetworkPacket q;
    q.write(OMIDOr<int32_t>());
    q.reset();
    auto c = q.read<OMIDOr<int32_t>>();
    CHECK(c.id == 0 && c.value == 0);
}

static auto testLpVec3() -> void
{
    // samples from the wiki: value -> expected bytes
    struct Sample
    {
        double x, y, z;
        std::vector<uint8_t> bytes;
    };

    const Sample samples[] = {
        {0.0, 0.0, 0.0, {0x00}},
        {1.0, 0.0, -1.0, {0xF1, 0xFF, 0x00, 0x00, 0xFF, 0xFF}},
        {10.0, 0.2, -5.0, {0xF6, 0xFF, 0x40, 0x01, 0x05, 0x1F, 0x02}},
    };

    for (const auto &sample : samples)
    {
        OMLpVec3 value;
        value.x = sample.x;
        value.y = sample.y;
        value.z = sample.z;

        OMNetworkPacket p;
        p.write(value);

        CHECK(p.datalen() == sample.bytes.size());

        const auto *bytes = reinterpret_cast<const uint8_t *>(p.data());

        for (std::size_t i = 0; i < sample.bytes.size() && i < p.datalen(); ++i)
        {
            CHECK(bytes[i] == sample.bytes[i]);
        }

        p.reset();
        auto parsed = p.read<OMLpVec3>();
        CHECK(std::abs(parsed.x - sample.x) < 0.01);
        CHECK(std::abs(parsed.y - sample.y) < 0.01);
        CHECK(std::abs(parsed.z - sample.z) < 0.01);
    }
}

static auto testAngle() -> void
{
    // one byte = 1/256 of a turn = 1.40625 degrees
    struct Sample
    {
        double degrees;
        uint8_t raw;
    };

    const Sample samples[] = {
        {0.0, 0x00}, {45.0, 0x20}, {90.0, 0x40}, {180.0, 0x80}, {270.0, 0xC0}, {-90.0, 0xC0},
    };

    for (const auto &sample : samples)
    {
        OMAngle angle(sample.degrees);
        CHECK(angle.raw() == sample.raw);

        OMNetworkPacket p;
        p.write(angle);
        CHECK(p.datalen() == 1);
        CHECK(static_cast<uint8_t>(p.data()[0]) == sample.raw);

        p.reset();
        auto parsed = p.read<OMAngle>();
        CHECK(std::abs(parsed.degrees - static_cast<double>(sample.raw) * (360.0 / 256.0)) < 1e-9);
    }

    // 360 degrees wraps to 0
    OMAngle fullTurn(360.0);
    CHECK(fullTurn.raw() == 0x00);
}

static auto testTeleportEntityCodec() -> void
{
    using openminecraft::shell::network::OMProtocolTeleportEntity;

    // linear path
    OMProtocolTeleportEntity linear;
    linear.entityId = 18898;
    linear.pathType = 0;
    linear.endX = -506.0983854342508;
    linear.endY = 13.0157000003336;
    linear.endZ = -224.42918151056918;
    linear.yaw = 272.39948f;
    linear.pitch = 15.5f;
    linear.onGround = true;

    OMNetworkPacket p;
    p.write(linear);
    p.reset();

    auto a = p.read<OMProtocolTeleportEntity>();
    CHECK(a.entityId == 18898 && a.pathType == 0);
    CHECK(a.endX == linear.endX && a.endY == linear.endY && a.endZ == linear.endZ);
    CHECK(a.yaw == linear.yaw && a.pitch == linear.pitch && a.onGround);
    CHECK(a.steps.empty());

    // stepped path
    OMProtocolTeleportEntity stepped;
    stepped.entityId = 42;
    stepped.pathType = 1;
    stepped.steps = {{1.0, 2.0, 3.0, 0}, {-1.5, 64.0, 7.25, 3}};
    stepped.onGround = false;

    OMNetworkPacket q;
    q.write(stepped);
    q.reset();

    auto b = q.read<OMProtocolTeleportEntity>();
    CHECK(b.pathType == 1 && b.steps.size() == 2);
    CHECK(b.steps[0].x == 1.0 && b.steps[0].y == 2.0 && b.steps[0].z == 3.0 && b.steps[0].tickOffset == 0);
    CHECK(b.steps[1].x == -1.5 && b.steps[1].tickOffset == 3);
}

static auto testEntityPositionCodecs() -> void
{
    using openminecraft::shell::network::OMProtocolUpdateEntityPosition;
    using openminecraft::shell::network::OMProtocolUpdateEntityPositionRotation;

    // movement: a Short on the wire, in 1/4096 of a block
    OMProtocolUpdateEntityPosition position;
    position.entityId = 19130;
    position.deltaX = 0.062255859375;  // 255 / 4096
    position.deltaY = -0.1875;         // -768 / 4096
    position.deltaZ = 5.8125;          // 23808 / 4096
    position.onGround = true;

    OMNetworkPacket p;
    p.write(position);
    p.reset();

    auto a = p.read<OMProtocolUpdateEntityPosition>();
    CHECK(a.entityId == 19130 && a.onGround);
    CHECK(a.deltaX == position.deltaX && a.deltaY == position.deltaY && a.deltaZ == position.deltaZ);

    // movement + rotation: yaw/pitch are Angle (1/256 of a turn)
    OMProtocolUpdateEntityPositionRotation rotated;
    rotated.entityId = 19158;
    rotated.deltaX = 0.18798828125;
    rotated.deltaY = 0.05810546875;
    rotated.deltaZ = 0.0;
    rotated.yaw = OMAngle(272.39948);
    rotated.pitch = OMAngle(97.03125);  // exactly 69 units
    rotated.onGround = true;

    OMNetworkPacket q;
    q.write(rotated);
    q.reset();

    auto b = q.read<OMProtocolUpdateEntityPositionRotation>();
    CHECK(b.entityId == 19158);
    CHECK(b.deltaX == rotated.deltaX && b.deltaY == rotated.deltaY && b.deltaZ == 0.0);
    CHECK(b.yaw.raw() == 194);   // round(272.39948 / 1.40625) & 0xFF
    CHECK(b.pitch.raw() == 69);
    CHECK(b.onGround);
}

static auto testSetHeadRotationCodec() -> void
{
    using openminecraft::shell::network::OMProtocolSetHeadRotation;

    OMProtocolSetHeadRotation rotation;
    rotation.entityId = 19428;
    rotation.headYaw = OMAngle(139.21875); // 99 Angle units

    OMNetworkPacket p;
    p.write(rotation);
    p.reset();

    auto parsed = p.read<OMProtocolSetHeadRotation>();
    CHECK(parsed.entityId == 19428);
    CHECK(parsed.headYaw.raw() == 0x63);
    CHECK(parsed.headYaw.degrees == 139.21875);
}

static auto testRemoveEntitiesCodec() -> void
{
    using openminecraft::shell::network::OMProtocolRemoveEntities;

    OMProtocolRemoveEntities entities;
    entities.entityIds = {19174, 42, 19428};

    OMNetworkPacket p;
    p.write(entities);
    p.reset();

    auto parsed = p.read<OMProtocolRemoveEntities>();
    CHECK(parsed.entityIds.size() == 3);
    CHECK(parsed.entityIds[0] == 19174 && parsed.entityIds[1] == 42 && parsed.entityIds[2] == 19428);

    OMProtocolRemoveEntities empty;
    OMNetworkPacket q;
    q.write(empty);
    q.reset();
    CHECK(q.read<OMProtocolRemoveEntities>().entityIds.empty());
}

static auto testSpawnEntityCodec() -> void
{
    using openminecraft::shell::network::OMProtocolSpawnEntity;

    // built byte by byte from a captured packet: entityId, uuid, type, x/y/z, velocity(LpVec3), pitch, yaw, headYaw, data
    OMNetworkPacket p;
    p.write(OMVarInt(20630));
    p.write(openminecraft::util::OMUuid(0x972665c8ccd6d99bULL, 0x7dd3f4d416f940a4ULL));
    p.write(OMVarInt(72));
    p.write<double>(-506.5).write<double>(13.0).write<double>(-222.8);
    p.write<uint8_t>(0xf9).write<uint8_t>(0xff).write<int32_t>(0x7FFEED19); // velocity LpVec3
    p.write<uint8_t>(0x00);                                                // pitch = 0
    p.write<uint8_t>(0x69);                                                // yaw
    p.write<uint8_t>(0x00);                                                // head yaw = 0
    p.write(OMVarInt(0));                                                  // data
    p.reset();

    auto parsed = p.read<OMProtocolSpawnEntity>();
    CHECK(parsed.entityId == 20630 && parsed.type == 72);
    CHECK(parsed.x == -506.5 && parsed.y == 13.0 && parsed.z == -222.8);
    CHECK(parsed.pitch.degrees == 0.0);
    CHECK(parsed.yaw.raw() == 0x69);
    CHECK(parsed.headYaw.degrees == 0.0);
    CHECK(parsed.data == 0);
    CHECK(p.restDatalen() == 0); // only the correct field order consumes the packet exactly
}

static auto testUpdateLightCodec() -> void
{
    using openminecraft::shell::network::OMProtocolUpdateLight;

    // masks use java.util.BitSet.toByteArray(): VarInt byte count + little-endian bytes
    OMNetworkPacket p;
    p.write(OMVarInt(-32)).write(OMVarInt(-14));
    p.write(OMVarInt(2));
    p.write<uint8_t>(0x00).write<uint8_t>(0x03);   // bit 8, 9
    p.write(OMVarInt(0));                           // block mask
    p.write(OMVarInt(0));                           // empty sky mask
    p.write(OMVarInt(0));                           // empty block mask
    p.write(OMVarInt(2));                           // two sky arrays
    for (int i = 0; i < 2; ++i)
    {
        p.write(OMVarInt(2048));
        p.writeFixedArray(std::vector<uint8_t>(2048, static_cast<uint8_t>(i + 1)));
    }
    p.write(OMVarInt(0));                           // zero block arrays
    p.reset();

    auto parsed = p.read<OMProtocolUpdateLight>();
    CHECK(parsed.chunkX == -32 && parsed.chunkZ == -14);
    CHECK(parsed.light.skyLightMask.count() == 2);
    CHECK(parsed.light.skyLightMask.test(8) && parsed.light.skyLightMask.test(9));
    CHECK(!parsed.light.skyLightMask.test(7));
    CHECK(parsed.light.skyLight.size() == 2 && parsed.light.skyLight[1][0] == 2);
    CHECK(parsed.light.blockLight.empty());
    CHECK(parsed.complete);

    // BitSet byte array round trip (java BitSet.toByteArray / BitSet.valueOf)
    OMBitSet mask;
    mask.data = {0x0000000000000300ULL}; // bit 8, 9
    CHECK(mask.toBytes() == std::vector<uint8_t>({0x00, 0x03}));
    CHECK(OMBitSet::fromBytes({0x00, 0x03}).count() == 2);
    CHECK(OMBitSet::fromBytes({}).data.empty());
}

static auto testIdSet() -> void
{
    // tag form
    OMIdSet tagSet;
    tagSet.type = 0;
    tagSet.tag = "minecraft:logs";

    OMNetworkPacket p;
    p.write(tagSet);
    p.reset();

    auto a = p.read<OMIdSet>();
    CHECK(a.type == 0 && a.isTag() && a.tag == "minecraft:logs" && a.ids.empty());

    // inline form: type = 3 means two ids
    OMIdSet inlineSet;
    inlineSet.type = 3;
    inlineSet.ids = {5, 7};

    OMNetworkPacket q;
    q.write(inlineSet);
    q.reset();

    auto b = q.read<OMIdSet>();
    CHECK(b.type == 3 && !b.isTag() && b.ids.size() == 2 && b.ids[0] == 5 && b.ids[1] == 7);

    // type = 1 means an empty set
    OMIdSet empty;
    empty.type = 1;

    OMNetworkPacket r;
    r.write(empty);
    r.reset();

    auto c = r.read<OMIdSet>();
    CHECK(c.type == 1 && c.ids.empty() && r.restDatalen() == 0);
}

static auto testSlotDisplay() -> void
{
    using openminecraft::shell::network::OMSlotDisplay;
    using openminecraft::shell::network::OMSlotDisplayKind;

    // minecraft:item
    OMNetworkPacket p;
    p.write(OMVarInt(static_cast<int32_t>(OMSlotDisplayKind::Item)));
    p.write(OMVarInt(42));
    p.reset();

    auto item = p.read<OMSlotDisplay>();
    CHECK(item.kind == OMSlotDisplayKind::Item && item.itemId == 42 && p.restDatalen() == 0);

    // minecraft:tag (no '#' prefix)
    OMNetworkPacket q;
    q.write(OMVarInt(static_cast<int32_t>(OMSlotDisplayKind::Tag)));
    q.write<std::string>("minecraft:logs");
    q.reset();

    auto tag = q.read<OMSlotDisplay>();
    CHECK(tag.kind == OMSlotDisplayKind::Tag && tag.tag == "minecraft:logs");

    // minecraft:dyed(dye, target)
    OMNetworkPacket r;
    r.write(OMVarInt(static_cast<int32_t>(OMSlotDisplayKind::Dyed)));
    r.write(OMVarInt(static_cast<int32_t>(OMSlotDisplayKind::Item)));
    r.write(OMVarInt(7));
    r.write(OMVarInt(static_cast<int32_t>(OMSlotDisplayKind::Item)));
    r.write(OMVarInt(9));
    r.reset();

    auto dyed = r.read<OMSlotDisplay>();
    CHECK(dyed.kind == OMSlotDisplayKind::Dyed && dyed.nested.size() == 2);
    CHECK(dyed.nested[0].itemId == 7 && dyed.nested[1].itemId == 9);

    // minecraft:composite (two nested entries)
    OMNetworkPacket c;
    c.write(OMVarInt(static_cast<int32_t>(OMSlotDisplayKind::Composite)));
    c.write(OMVarInt(2));
    c.write(OMVarInt(static_cast<int32_t>(OMSlotDisplayKind::Empty)));
    c.write(OMVarInt(static_cast<int32_t>(OMSlotDisplayKind::Item)));
    c.write(OMVarInt(3));
    c.reset();

    auto composite = c.read<OMSlotDisplay>();
    CHECK(composite.kind == OMSlotDisplayKind::Composite && composite.nested.size() == 2);
    CHECK(composite.nested[0].kind == OMSlotDisplayKind::Empty && composite.nested[1].itemId == 3);

    // minecraft:only_with_component（base + component type id）
    OMNetworkPacket o;
    o.write(OMVarInt(static_cast<int32_t>(OMSlotDisplayKind::OnlyWithComponent)));
    o.write(OMVarInt(static_cast<int32_t>(OMSlotDisplayKind::Item)));
    o.write(OMVarInt(1));
    o.write(OMVarInt(77));
    o.reset();

    auto only = o.read<OMSlotDisplay>();
    CHECK(only.nested.size() == 1 && only.nested[0].itemId == 1 && only.componentTypeId == 77);

    // minecraft:item_stack (empty stack)
    OMNetworkPacket st;
    st.write(OMVarInt(static_cast<int32_t>(OMSlotDisplayKind::ItemStack)));
    st.write(OMVarInt(0));
    st.reset();

    auto stack = st.read<OMSlotDisplay>();
    CHECK(stack.kind == OMSlotDisplayKind::ItemStack && stack.stack.count == 0 && !stack.truncated);
}

// helper: build a mapping holding only the components used by a test (ids are explicit)
static auto makeMapping(std::initializer_list<std::pair<int32_t, const char *>> entries)
    -> openminecraft::shell::network::OMDataComponentMapping
{
    std::vector<std::string> names;

    for (const auto &[id, name] : entries)
    {
        if (static_cast<int32_t>(names.size()) <= id)
        {
            names.resize(static_cast<std::size_t>(id) + 1);
        }

        names[static_cast<std::size_t>(id)] = name;
    }

    openminecraft::shell::network::OMDataComponentMapping mapping;
    mapping.setNames(std::move(names));
    return mapping;
}

static auto testDataComponents() -> void
{
    using openminecraft::shell::network::OMDataComponentMapping;
    using openminecraft::shell::network::OMDataComponentHandler;
    using openminecraft::shell::network::OMProtocolDataComponent;

    // damage (id 3): VarInt payload
    {
        auto mapping = makeMapping({{3, "minecraft:damage"}});

        OMNetworkPacket p;
        p.write(OMVarInt(3)).write(OMVarInt(100));
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::Damage && component.intValue == 100);
    }

    // custom_name (id 6): network NBT string without root name
    {
        auto mapping = makeMapping({{6, "minecraft:custom_name"}});

        OMNetworkPacket p;
        p.write(OMVarInt(6));
        p.write<uint8_t>(0x08).write(OMVarInt(0)).write(OMVarInt(15)); // tag, name len, value len
        for (char c : std::string("{\"text\":\"test\"}"))
        {
            p.write<uint8_t>(static_cast<uint8_t>(c));
        }
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::CustomName);
        CHECK(component.stringValue == "{\"text\":\"test\"}");
    }

    // lore (id 11): count + text components
    {
        auto mapping = makeMapping({{11, "minecraft:lore"}});

        OMNetworkPacket p;
        p.write(OMVarInt(11)).write(OMVarInt(2));
        for (const char *line : {"line1", "line2"})
        {
            auto text = std::string(line);
            p.write<uint8_t>(0x08).write(OMVarInt(0)).write(OMVarInt(static_cast<int32_t>(text.size())));
            for (char c : text)
            {
                p.write<uint8_t>(static_cast<uint8_t>(c));
            }
        }
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::Lore && component.stringList.size() == 2);
        CHECK(component.stringList[0] == "line1" && component.stringList[1] == "line2");
    }

    // enchantments (id 13): count + (id, level) pairs
    {
        auto mapping = makeMapping({{13, "minecraft:enchantments"}});

        OMNetworkPacket p;
        p.write(OMVarInt(13)).write(OMVarInt(2));
        p.write(OMVarInt(5)).write(OMVarInt(3));  // sharpness 3
        p.write(OMVarInt(7)).write(OMVarInt(2));  // unbreaking 2
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::Enchantments);
        CHECK(component.intList.size() == 2 && component.intList[0] == 5 && component.intList[1] == 7);
        CHECK(component.intListB.size() == 2 && component.intListB[0] == 3 && component.intListB[1] == 2);
    }

    // attribute_modifiers (id 16)
    {
        auto mapping = makeMapping({{16, "minecraft:attribute_modifiers"}});

        OMNetworkPacket p;
        p.write(OMVarInt(16)).write(OMVarInt(1));
        p.write(OMVarInt(2));                                      // attribute id
        p.write<std::string>("minecraft:base_attack_damage");      // modifier id
        p.write<double>(7.5);                                      // value
        p.write(OMVarInt(0)).write(OMVarInt(1));                   // operation, slot
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::AttributeModifiers);
        CHECK(component.attributeModifiers.size() == 1);
        CHECK(component.attributeModifiers[0].attributeId == 2);
        CHECK(component.attributeModifiers[0].id == "minecraft:base_attack_damage");
        CHECK(component.attributeModifiers[0].value == 7.5);
        CHECK(component.attributeModifiers[0].operation == 0 && component.attributeModifiers[0].slot == 1);
    }

    // use_cooldown (id 26): Float + optional Identifier
    {
        auto mapping = makeMapping({{26, "minecraft:use_cooldown"}});

        OMNetworkPacket p;
        p.write(OMVarInt(26)).write<float>(0.5f).write<bool>(true).write<std::string>("minecraft:test");
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::UseCooldown);
        CHECK(component.floatValue == 0.5f && component.optionalString == "minecraft:test");
    }

    // tool (id 28): one rule + default speed + damage per block + creative flag
    {
        auto mapping = makeMapping({{28, "minecraft:tool"}});

        OMNetworkPacket p;
        p.write(OMVarInt(28)).write(OMVarInt(1));   // one rule
        p.write(OMVarInt(3)).write(OMVarInt(1)).write(OMVarInt(9)); // id set: 1 inline id (9)
        p.write<bool>(true).write<float>(8.0f);      // optional speed
        p.write<bool>(false);                        // no correct-drop override
        p.write<float>(2.0f);                        // default mining speed
        p.write(OMVarInt(1));                        // damage per block
        p.write<bool>(true);                         // can destroy in creative
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::Tool);
        CHECK(component.toolRules.size() == 1 && component.floatValue == 2.0f);
        CHECK(component.intValue == 1 && component.boolValue);
    }

    // unknown name -> truncated, no overrun
    {
        auto mapping = makeMapping({{99, "minecraft:unknown"}});

        OMNetworkPacket p;
        p.write(OMVarInt(99)).write<uint8_t>(0xAA).write<uint8_t>(0xBB);
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.truncated);
        CHECK(!component.head.empty());   // raw head captured for later analysis
    }
}

static auto testMoreDataComponents() -> void
{
    using openminecraft::shell::network::OMDataComponentHandler;
    using openminecraft::shell::network::OMProtocolDataComponent;

    // potion_contents (53): optional id | optional color | effects | optional name
    {
        auto mapping = makeMapping({{53, "minecraft:potion_contents"}});

        OMNetworkPacket p;
        p.write(OMVarInt(53));
        p.write<bool>(true).write(OMVarInt(3));          // potion id
        p.write<bool>(true).write<int32_t>(0xFF0000);    // custom color
        p.write(OMVarInt(1));                            // one effect
        p.write(OMVarInt(7));                            // type id
        p.write(OMVarInt(1)).write(OMVarInt(200));       // amplifier, duration
        p.write<bool>(true).write<bool>(true).write<bool>(false); // ambient, particles, icon
        p.write<bool>(false);                            // no hidden effect
        p.write<bool>(false);                            // no custom name
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::PotionContents);
        CHECK(component.potionId == 3 && component.potionColor == 0xFF0000);
        CHECK(component.potionEffects.size() == 1 && component.potionEffects[0].typeId == 7);
    }

    // firework_explosion (70): shape | colors | fade | trail | twinkle
    {
        auto mapping = makeMapping({{70, "minecraft:firework_explosion"}});

        OMNetworkPacket p;
        p.write(OMVarInt(70));
        p.write(OMVarInt(2));                            // shape = star
        p.write(OMVarInt(1)).write<int32_t>(0x112233);   // colors
        p.write(OMVarInt(1)).write<int32_t>(0x445566);   // fade colors
        p.write<bool>(true).write<bool>(false);          // trail, twinkle
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::FireworkExplosion);
        CHECK(component.fireworkExplosion.shape == 2);
        CHECK(component.fireworkExplosion.colors.size() == 1 && component.fireworkExplosion.colors[0] == 0x112233);
        CHECK(component.fireworkExplosion.hasTrail && !component.fireworkExplosion.hasTwinkle);
    }

    // fireworks (71): flight duration + explosion array
    {
        auto mapping = makeMapping({{71, "minecraft:fireworks"}});

        OMNetworkPacket p;
        p.write(OMVarInt(71)).write(OMVarInt(2));        // flight
        p.write(OMVarInt(1));                            // one explosion
        p.write(OMVarInt(0));                            // shape
        p.write(OMVarInt(0)).write(OMVarInt(0));         // no colors
        p.write<bool>(false).write<bool>(false);
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::Fireworks);
        CHECK(component.intValue == 2 && component.fireworkExplosions.size() == 1);
    }

    // trim (58): ID or Trim Material + ID or Trim Pattern (registry references)
    {
        auto mapping = makeMapping({{58, "minecraft:trim"}});

        OMNetworkPacket p;
        p.write(OMVarInt(58));
        p.write(OMVarInt(2));                            // material: registry id 1
        p.write(OMVarInt(3));                            // pattern: registry id 2
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::Trim);
        CHECK(!component.trimMaterial.isInline() && component.trimMaterial.registryId() == 1);
        CHECK(!component.trimPattern.isInline() && component.trimPattern.registryId() == 2);
    }

    // consumable (24): seconds | animation | sound | particles | effects
    {
        auto mapping = makeMapping({{24, "minecraft:consumable"}});

        OMNetworkPacket p;
        p.write(OMVarInt(24));
        p.write<float>(1.6f);                            // consume seconds
        p.write(OMVarInt(1));                            // animation = eat
        p.write(OMVarInt(1));                            // sound: registry id 0
        p.write<bool>(true);                             // has particles
        p.write(OMVarInt(1));                            // one effect
        p.write(OMVarInt(2));                            // clear_all_effects
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::Consumable);
        CHECK(component.floatValue == 1.6f && component.intValue == 1 && component.boolValue);
        CHECK(component.intList.size() == 1 && component.intList[0] == 2);
    }

    // can_place_on (14): one predicate with a blocks id set only
    {
        auto mapping = makeMapping({{14, "minecraft:can_place_on"}});

        OMNetworkPacket p;
        p.write(OMVarInt(14));
        p.write(OMVarInt(1));                            // one predicate
        p.write<bool>(true);                             // blocks present
        p.write(OMVarInt(2)).write(OMVarInt(5));         // id set: one inline id (5)
        p.write<bool>(false);                            // no properties
        p.write<bool>(false);                            // no nbt
        p.write(OMVarInt(0));                            // no exact matchers
        p.write(OMVarInt(0));                            // no partial predicates
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::CanPlaceOn);
        CHECK(component.idSet.ids.size() == 1 && component.idSet.ids[0] == 5);
    }
}

static auto testNewComponents() -> void
{
    using openminecraft::shell::network::OMDataComponentHandler;
    using openminecraft::shell::network::OMProtocolDataComponent;

    // villager_food (44): VarInt
    {
        auto mapping = makeMapping({{44, "minecraft:villager_food"}});

        OMNetworkPacket p;
        p.write(OMVarInt(44)).write(OMVarInt(6));
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::VillagerFood && component.intValue == 6);
    }

    // mob_visibility (87): ID Set + Float
    {
        auto mapping = makeMapping({{87, "minecraft:mob_visibility"}});

        OMNetworkPacket p;
        p.write(OMVarInt(87));
        p.write(OMVarInt(2)).write(OMVarInt(4));   // one inline entity id
        p.write<float>(2.5f);
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::MobVisibility);
        CHECK(component.idSet.ids.size() == 1 && component.idSet.ids[0] == 4);
        CHECK(component.floatValue == 2.5f);
    }

    // piercing_weapon (38): two booleans + two optional sounds (absent)
    {
        auto mapping = makeMapping({{38, "minecraft:piercing_weapon"}});

        OMNetworkPacket p;
        p.write(OMVarInt(38)).write<bool>(true).write<bool>(false).write<bool>(false).write<bool>(false);
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::PiercingWeapon);
        CHECK(component.boolValue && !component.secondBool);
        CHECK(component.intList.empty() && component.intListB.empty());
    }

    // lodestone_tracker (69): no position + tracked
    {
        auto mapping = makeMapping({{69, "minecraft:lodestone_tracker"}});

        OMNetworkPacket p;
        p.write(OMVarInt(69)).write<bool>(false).write<bool>(true);
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::LodestoneTracker);
        CHECK(!component.boolValue && component.secondBool);
    }

    // banner_patterns (74): one layer (pattern id + dye color)
    {
        auto mapping = makeMapping({{74, "minecraft:banner_patterns"}});

        OMNetworkPacket p;
        p.write(OMVarInt(74)).write(OMVarInt(1)).write(OMVarInt(3)).write(OMVarInt(14));
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::BannerPatterns);
        CHECK(component.intList.size() == 1 && component.intList[0] == 3);
        CHECK(component.intListB.size() == 1 && component.intListB[0] == 14);
    }

    // bees (79): one bee (entity type + NBT + ticks + min ticks)
    {
        auto mapping = makeMapping({{79, "minecraft:bees"}});

        OMNetworkPacket p;
        p.write(OMVarInt(79)).write(OMVarInt(1));
        p.write(OMVarInt(5));                                // entity type
        p.write<uint8_t>(0x08).write(OMVarInt(0)).write(OMVarInt(1)).write<uint8_t>('b'); // tiny NBT string
        p.write(OMVarInt(20)).write(OMVarInt(10));           // ticks, min ticks
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::Bees);
        CHECK(component.intList.size() == 1 && component.intList[0] == 5);
        CHECK(component.intListB[0] == 20 && component.intListC[0] == 10);
    }

    // sign_text_front (118): 4 lines, no filtered lines, color + glowing
    {
        auto mapping = makeMapping({{118, "minecraft:sign_text_front"}});

        OMNetworkPacket p;
        p.write(OMVarInt(118));

        for (int i = 0; i < 4; ++i)
        {
            p.write<uint8_t>(0x08).write(OMVarInt(0)).write(OMVarInt(1)).write<uint8_t>('x'); // one-char line
        }

        p.write<bool>(false);      // no filtered lines
        p.write(OMVarInt(3));      // dye color
        p.write<bool>(true);       // glowing
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::SignTextFront);
        CHECK(component.signMessages.size() == 4 && component.signFiltered.empty());
        CHECK(component.intValue == 3 && component.boolValue);
    }

    // equippable (32): slot + sound + 3 absent optionals + 4 booleans + shearing sound
    {
        auto mapping = makeMapping({{32, "minecraft:equippable"}});

        OMNetworkPacket p;
        p.write(OMVarInt(32));
        p.write(OMVarInt(4));            // slot = head
        p.write(OMVarInt(1));            // equip sound: registry id 0
        p.write<bool>(false);            // no model
        p.write<bool>(false);            // no camera overlay
        p.write<bool>(false);            // no allowed entities
        p.write<bool>(true);             // dispensable
        p.write<bool>(false);            // swappable
        p.write<bool>(true);             // damage on hurt
        p.write<bool>(false);            // can be sheared
        p.write(OMVarInt(2));            // shearing sound: registry id 1
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::Equippable);
        CHECK(component.intValue == 4 && component.boolValue && !component.secondBool);
        CHECK(component.intList[0] == 1 && component.intListB[0] == 0);
    }
}

static auto testResolvableNumbers() -> void
{
    // constant float: 0x00 + Float
    OMResolvableFloat constant;
    constant.isReference = false;
    constant.value = 2.5f;

    OMNetworkPacket p;
    p.write(constant);
    p.reset();

    auto a = p.read<OMResolvableFloat>();
    CHECK(!a.isReference && a.value == 2.5f && a.key.empty());

    // reference float: 0x01 + Identifier
    OMResolvableFloat reference;
    reference.isReference = true;
    reference.key = "minecraft:test";

    OMNetworkPacket q;
    q.write(reference);
    q.reset();

    auto b = q.read<OMResolvableFloat>();
    CHECK(b.isReference && b.key == "minecraft:test");

    // constant / reference int
    OMResolvableInt intConstant;
    intConstant.value = 42;

    OMResolvableInt intReference;
    intReference.isReference = true;
    intReference.key = "minecraft:other";

    OMNetworkPacket r;
    r.write(intConstant);
    r.write(intReference);
    r.reset();

    auto c = r.read<OMResolvableInt>();
    CHECK(!c.isReference && c.value == 42);

    auto d = r.read<OMResolvableInt>();
    CHECK(d.isReference && d.key == "minecraft:other");
}

static auto testResolvableComponents() -> void
{
    using openminecraft::shell::network::OMDataComponentHandler;
    using openminecraft::shell::network::OMProtocolDataComponent;

    // cooking_fuel (85): resolvable int burn time + resolvable float speed
    {
        auto mapping = makeMapping({{85, "minecraft:cooking_fuel"}});

        OMNetworkPacket p;
        p.write(OMVarInt(85));
        p.write<bool>(false).write(OMVarInt(200));                 // burn time constant
        p.write<bool>(true).write<std::string>("minecraft:fuel");  // speed multiplier reference
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::CookingFuel);
        CHECK(!component.cookingBurnTime.isReference && component.cookingBurnTime.value == 200);
        CHECK(component.cookingSpeedMultiplier.isReference &&
              component.cookingSpeedMultiplier.key == "minecraft:fuel");
    }

    // brewing_fuel (86): resolvable int uses + resolvable float speed
    {
        auto mapping = makeMapping({{86, "minecraft:brewing_fuel"}});

        OMNetworkPacket p;
        p.write(OMVarInt(86));
        p.write<bool>(true).write<std::string>("minecraft:uses");  // uses reference
        p.write<bool>(false).write<float>(1.5f);                   // speed constant
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::BrewingFuel);
        CHECK(component.brewingUses.isReference && component.brewingUses.key == "minecraft:uses");
        CHECK(!component.brewingSpeedMultiplier.isReference && component.brewingSpeedMultiplier.value == 1.5f);
    }

    // compostable (84): resolvable int layers
    {
        auto mapping = makeMapping({{84, "minecraft:compostable"}});

        OMNetworkPacket p;
        p.write(OMVarInt(84)).write<bool>(false).write(OMVarInt(7));
        p.setUserContext(&mapping);
        p.reset();

        auto component = p.read<OMProtocolDataComponent>();
        CHECK(component.type == OMDataComponentHandler::Compostable);
        CHECK(!component.compostLayers.isReference && component.compostLayers.value == 7);
    }
}

static auto testOptionalCodec() -> void
{
    OMNetworkPacket p;
    p.write(std::optional<TestPair>{TestPair{5, 6}});
    p.write(std::optional<TestPair>{});
    p.write(std::optional<int32_t>{42});
    p.write(std::optional<std::string>{"hi"});
    p.reset();

    auto present = p.read<std::optional<TestPair>>();
    CHECK(present.has_value() && present->left == 5 && present->right == 6);

    auto absent = p.read<std::optional<TestPair>>();
    CHECK(!absent.has_value());

    auto number = p.read<std::optional<int32_t>>();
    CHECK(number.has_value() && *number == 42);

    auto text = p.read<std::optional<std::string>>();
    CHECK(text.has_value() && *text == "hi");
}

static auto testStringVectorConversion() -> void
{
    std::vector<OMUtf8String> utf8 = {OMUtf8String("x"), OMUtf8String("yy")};

    auto asStrings = toStdStrings(utf8);
    CHECK(asStrings.size() == 2 && asStrings[0] == "x" && asStrings[1] == "yy");

    auto backToUtf8 = toUtf8Strings(asStrings);
    CHECK(backToUtf8.size() == 2 && backToUtf8[1].value == "yy");
}

auto main() -> int
{
    testPrefixArrays();
    testEmptyArray();
    testScalars();
    testWireFormat();
    testCustomCodec();
    testFixedAndPrefixArrayWrites();
    testProtocolCodecs();
    testGameProfileCodec();
    testUpdateTagsCodec();
    testVarLong();
    testCustomReportDetailsCodec();
    testSetTimeCodec();
    testPackedPos();
    testSpawnPositionCodec();
    testPostEffectsCodec();
    testContainerContentCodec();
    testUpdateAttributesCodec();
    testUpdateAdvancementsCodec();
    testSetHealthCodec();
    testSetExperienceCodec();
    testBitSet();
    testIDOr();
    testIdSet();
    testSlotDisplay();
    testDataComponents();
    testMoreDataComponents();
    testNewComponents();
    testResolvableNumbers();
    testResolvableComponents();
    testLpVec3();
    testAngle();
    testTeleportEntityCodec();
    testEntityPositionCodecs();
    testSetHeadRotationCodec();
    testRemoveEntitiesCodec();
    testSpawnEntityCodec();
    testUpdateLightCodec();
    testOptionalCodec();
    testStringVectorConversion();

    if (failures)
    {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }

    std::puts("om_network_packet_test: all checks passed");
    return 0;
}
