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

// 扩展点示例：自定义类型只需要特化 OMNetworkPacketCodec<T>，无需改动 OMNetworkPacket
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

    // std::string[2]（OMUtf8String 的自然类型版本）
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
    // 标量
    OMNetworkPacket p;
    p.write(TestPair{7, -9});
    p.reset();

    auto pair = p.read<TestPair>();
    CHECK(pair.left == 7 && pair.right == -9);

    // 数组：readPrefixArray 复用 read<TestPair>()，同样自动支持
    OMNetworkPacket a;
    a.write(OMVarInt(2)).write(TestPair{1, 2}).write(TestPair{3, 4});
    a.reset();

    auto list = a.readPrefixArray<TestPair>();
    CHECK(list.size() == 2 && list[0].left == 1 && list[1].right == 4);
}

static auto testFixedAndPrefixArrayWrites() -> void
{
    // 定长数组：线上没有长度前缀
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

    // 前缀数组：先 VarInt 元素个数
    OMNetworkPacket prefixed;
    prefixed.writePrefixArray<int32_t>({1, -2, 3});
    prefixed.writePrefixArray<OMUtf8String>({OMUtf8String("aa"), OMUtf8String("bb")});
    prefixed.writePrefixArray<uint8_t>({0x11, 0x22});

    // 线上第一字节应当是元素个数 3
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

    // 线格式：300 -> AC 02；-1 -> FF x9 + 01（共 12 字节）
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

    // 数组与隐式转换
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

    // 边界值：26 位与 12 位补码的两端
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
    p.write(OMVarInt(3)).write(OMVarInt(5)).write(OMVarInt(0)).write(OMVarInt(1)).write(OMVarInt(7));
    p.write(OMVarInt(0)); // carried item: empty
    p.reset();

    auto parsed = p.read<OMProtocolSetContainerContent>();
    CHECK(parsed.windowId == 0 && parsed.stateId == 1 && parsed.slots.size() == 2);
    CHECK(parsed.slots[0].count == 0);
    CHECK(parsed.slots[1].count == 3 && parsed.slots[1].itemId == 5);
    CHECK(parsed.slots[1].removedComponents.size() == 1 && parsed.slots[1].removedComponents[0] == 7);
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
    referenced.id = 3; // 注册表 id 2 + 1

    OMNetworkPacket p;
    p.write(inlineValue);
    p.write(referenced);
    p.reset();

    auto a = p.read<OMIDOr<OMUtf8String>>();
    CHECK(a.id == 0 && a.isInline() && a.value && a.value->value == "minecraft:test");

    auto b = p.read<OMIDOr<OMUtf8String>>();
    CHECK(b.id == 3 && !b.isInline() && b.registryId() == 2 && !b.value);

    // 也支持直接内联基本类型
    OMNetworkPacket q;
    q.write(OMIDOr<int32_t>());
    q.reset();
    auto c = q.read<OMIDOr<int32_t>>();
    CHECK(c.id == 0 && c.value == 0);
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
