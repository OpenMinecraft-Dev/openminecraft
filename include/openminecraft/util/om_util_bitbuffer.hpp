#ifndef OM_UTIL_BITBUFFER_HPP
#define OM_UTIL_BITBUFFER_HPP

#include <cstdint>
#include <stdexcept>
namespace openminecraft::util
{
class OMBitBuffer
{
  public:
    OMBitBuffer() = default;
    ~OMBitBuffer() = default;

    void push(uint8_t data)
    {
        checkBit(8);
        buffer = buffer << 8 | data;
        bits += 8;
    }

    auto popBit() -> bool
    {
        bits--;
        bool l = (buffer >> bits) & 1;
        buffer &= ((1 << bits) - 1);
        return l;
    }

    auto popValue(int8_t bits) -> uint32_t
    {
        if (!bits)
        {
            return 0;
        }
        uint32_t result = buffer >> (this->bits - bits) & ((1 << bits) - 1);
        this->bits -= bits;
        buffer &= ((1 << this->bits) - 1);
        return result;
    }

    void checkBit(int8_t needed)
    {
        if (bits + needed > 32)
        {
            throw std::logic_error("too many bits!");
        }
    }

    auto bitsAvailable() -> int8_t
    {
        return bits;
    }

    uint32_t buffer = 0;

  private:
    int8_t bits = 0;
};
}; // namespace openminecraft::util

#endif
