#include <openminecraft/binary/om_bin_endians.hpp>

namespace openminecraft::binary
{
auto be16ToNative(uint16_t data) -> uint16_t
{
    return checkNativeLe() ? ((data & 0x00ff) << 8) | ((data & 0xff00) >> 8) : data;
}

auto be32ToNative(uint32_t data) -> uint32_t
{
    return checkNativeLe() ? ((data & 0x000000ff) << 24) | ((data & 0x0000ff00) << 8) | ((data & 0x00ff0000) >> 8) |
                                 ((data & 0xff000000) >> 24)
                           : data;
}

auto be64ToNative(uint64_t data) -> uint64_t
{
    return checkNativeLe() ? ((data & 0x00000000000000ff) << 56) | ((data & 0x000000000000ff00) << 40) |
                                 ((data & 0x0000000000ff0000) << 24) | ((data & 0x00000000ff000000) << 8) |
                                 ((data & 0x000000ff00000000) >> 8) | ((data & 0x0000ff0000000000) >> 24) |
                                 ((data & 0x00ff000000000000) >> 40) | ((data & 0xff00000000000000) >> 56)
                           : data;
}
auto le16ToNative(uint16_t data) -> uint16_t
{
    return !checkNativeLe() ? ((data & 0x00ff) << 8) | ((data & 0xff00) >> 8) : data;
}

auto le32ToNative(uint32_t data) -> uint32_t
{
    return !checkNativeLe() ? ((data & 0x000000ff) << 24) | ((data & 0x0000ff00) << 8) | ((data & 0x00ff0000) >> 8) |
                                  ((data & 0xff000000) >> 24)
                            : data;
}

auto le64ToNative(uint64_t data) -> uint64_t
{
    return !checkNativeLe() ? ((data & 0x00000000000000ff) << 56) | ((data & 0x000000000000ff00) << 40) |
                                  ((data & 0x0000000000ff0000) << 24) | ((data & 0x00000000ff000000) << 8) |
                                  ((data & 0x000000ff00000000) >> 8) | ((data & 0x0000ff0000000000) >> 24) |
                                  ((data & 0x00ff000000000000) >> 40) | ((data & 0xff00000000000000) >> 56)
                            : data;
}

auto befToNative(float data) -> float
{
    union {
        uint32_t idata;
        float fdata;
    } d;

    d.fdata = data;
    d.idata = be32ToNative(d.idata);

    return d.fdata;
}

auto be16SignedToNative(uint8_t d1, uint8_t d2) -> int16_t
{
    return (int16_t)(((int8_t)d1) << 8 | ((int8_t)d2));
}

auto be32SignedToNative(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4) -> int32_t
{
    return (int16_t)(((int8_t)d1) << 24 | ((int8_t)d2) << 16 | ((int8_t)d3) << 8 | ((uint8_t)d4));
}

auto bedToNative(double data) -> double
{
    union {
        uint64_t ldata;
        double ddata;
    } d;

    d.ddata = data;
    d.ldata = be32ToNative(d.ldata);

    return d.ddata;
}

auto checkNativeLe() -> bool
{
    union {
        uint16_t full;
        uint8_t base;
    } d;

    d.full = 0x1234;
    return d.base == 0x34;
}
} // namespace openminecraft::binary
