#ifndef OM_NETWORK_SOCKETSTREAM_HPP
#define OM_NETWORK_SOCKETSTREAM_HPP

#include <vector>
namespace openminecraft::network
{
class OMNetworkPacket
{
  public:
    OMNetworkPacket() = default;

    auto datalen() -> uint64_t
    {
        return buffer.size();
    }

    auto data() -> uint8_t *
    {
        return buffer.data();
    }

    auto varInt(uint32_t value) -> OMNetworkPacket &
    {
        while ((value & ~0x7F) != 0)
        {
            buffer.push_back((value & 0x7F) | 0x80);
            value >>= 7;
        }
        buffer.push_back(value);
        return *this;
    }

    auto utf8WithPrefix(std::string s) -> OMNetworkPacket &
    {
        varInt(s.size());
        for (auto ch : s)
        {
            buffer.push_back(ch);
        }
        return *this;
    }

    auto shortInt(int16_t t) -> OMNetworkPacket &
    {
        buffer.push_back((t << 8) & 0xff);
        buffer.push_back(t & 0xff);
        return *this;
    }

  private:
    std::vector<uint8_t> buffer;
};
} // namespace openminecraft::network

#endif