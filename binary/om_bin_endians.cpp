#include <openminecraft/binary/om_bin_endians.hpp>

namespace openminecraft::binary {
uint16_t be16ToNative(uint16_t data)
{
    return checkNativeLe() ? ((data & 0x00ff) << 8) | ((data & 0xff00) >> 8) : data;
}

uint32_t be32ToNative(uint32_t data)
{
    return checkNativeLe() ? ((data & 0x000000ff) << 24) | ((data & 0x0000ff00) << 8) | ((data & 0x00ff0000) >> 8) | ((data & 0xff000000) >> 24) : data;
}

uint64_t be64ToNative(uint64_t data)
{
    return checkNativeLe() ? ((data & 0x00000000000000ff) << 56) | ((data & 0x000000000000ff00) << 40) | ((data & 0x0000000000ff0000) << 24) | ((data & 0x00000000ff000000) << 8) | ((data & 0x000000ff00000000) >> 8) | ((data & 0x0000ff0000000000) >> 24) | ((data & 0x00ff000000000000) >> 40) | ((data & 0xff00000000000000) >> 56) : data;
}

float befToNative(float data)
{
    union {
        uint32_t idata;
        float fdata;
    } d;

    d.fdata = data;
    d.idata = be32ToNative(d.idata);

    return d.fdata;
}

double bedToNative(double data)
{
    union {
        uint64_t ldata;
        double ddata;
    } d;

    d.ddata = data;
    d.ldata = be32ToNative(d.ldata);

    return d.ddata;
}

bool checkNativeLe()
{
    union {
        uint16_t full;
        uint8_t base;
    } d;

    d.full = 0x1234;
    return d.base == 0x34;
}
}