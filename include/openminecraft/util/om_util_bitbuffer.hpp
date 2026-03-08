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

    bool popBit()
    {
        bits--;
        return (buffer >> bits) & 1;
    }

    uint32_t popValue(int8_t bits)
    {
        uint32_t result = buffer >> (this->bits - bits) & ((1 << bits) - 1);
        buffer >>= (this->bits - bits);
        this->bits -= bits;
        return result;
    }

    void checkBit(int8_t needed)
    {
        if (bits + needed > 32)
        {
            throw std::logic_error("too many bits!");
        }
    }

    int8_t bitsAvailable()
    {
        return bits;
    }

  private:
    uint32_t buffer = 0;
    int8_t bits = 0;
};
}; // namespace openminecraft::util

#endif
