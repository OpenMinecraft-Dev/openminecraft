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

    auto reset() -> OMNetworkPacket &
    {
        offset = 0;
        return *this;
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

    auto utf8WithLength(std::string s) -> OMNetworkPacket &
    {
        varInt(s.size());
        for (auto ch : s)
        {
            buffer.push_back(ch);
        }
        return *this;
    }

    auto int16(int16_t t) -> OMNetworkPacket &
    {
        buffer.push_back((t >> 8) & 0xff);
        buffer.push_back(t & 0xff);
        return *this;
    }

    auto int32(int32_t t) -> OMNetworkPacket &
    {
        buffer.push_back((t >> 24) & 0xff);
        buffer.push_back((t >> 16) & 0xff);
        buffer.push_back((t >> 8) & 0xff);
        buffer.push_back(t & 0xff);

        return *this;
    }

    auto int64(int64_t t) -> OMNetworkPacket &
    {
        buffer.push_back((t >> 56) & 0xff);
        buffer.push_back((t >> 48) & 0xff);
        buffer.push_back((t >> 40) & 0xff);
        buffer.push_back((t >> 32) & 0xff);
        buffer.push_back((t >> 24) & 0xff);
        buffer.push_back((t >> 16) & 0xff);
        buffer.push_back((t >> 8) & 0xff);
        buffer.push_back(t & 0xff);

        return *this;
    }

    auto assign(std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end)
        -> OMNetworkPacket &
    {
        buffer.assign(begin, end);
        return *this;
    }

  private:
    std::vector<uint8_t> buffer;
    uint64_t offset;
};
} // namespace openminecraft::network

#endif