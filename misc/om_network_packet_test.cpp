//   FMT_INC=$(find "$HOME/.xmake/packages/f/fmt" -type d -name include | head -1)
//   c++ -std=c++17 -DFMT_HEADER_ONLY -I include -I "$FMT_INC" \
//       misc/om_network_packet_test.cpp -o /tmp/om_network_packet_test
//   /tmp/om_network_packet_test

#include "openminecraft/network/om_network_packet.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

using namespace openminecraft::network;

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
    CHECK(ids.size() == 2 && ids[0].highBits() == 0x0102030405060708LL && ids[1].lowBits() == 0LL);
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
    testStringVectorConversion();

    if (failures)
    {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }

    std::puts("om_network_packet_test: all checks passed");
    return 0;
}
