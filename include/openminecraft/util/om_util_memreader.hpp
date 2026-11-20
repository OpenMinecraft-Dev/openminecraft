#ifndef OM_UTIL_MEMREADER_HPP
#define OM_UTIL_MEMREADER_HPP

#include <cstdint>
#include <vector>
#include <cstring>

namespace openminecraft::util
{

class OMMemoryReader
{
    const uint8_t *data;
    const uint8_t *pos;
    const uint8_t *end;

  public:
    OMMemoryReader(const std::vector<uint8_t> &buffer) : data(buffer.data()), pos(data), end(data + buffer.size())
    {
    }

    inline auto require(size_t) -> void
    {
    }
    inline auto raw() -> const uint8_t *
    {
        return pos;
    }

    inline auto readu8() -> uint8_t
    {
        return *pos++;
    }
    inline auto readu16() -> uint16_t
    {
        uint16_t v = (pos[0] << 8) | pos[1];
        pos += 2;
        return v;
    }
    inline auto readu32() -> uint32_t
    {
        uint32_t v = (pos[0] << 24) | (pos[1] << 16) | (pos[2] << 8) | pos[3];
        pos += 4;
        return v;
    }
    inline auto readu64() -> uint64_t
    {
        uint64_t v = readu32();
        v <<= 32;
        v |= readu32();
        return v;
    }
    inline void skip(size_t n)
    {
        pos += n;
    }

    inline void readn(void *target, int n)
    {
        std::memcpy(target, pos, n);
        pos += n;
    }
};
} // namespace openminecraft::util

#endif
