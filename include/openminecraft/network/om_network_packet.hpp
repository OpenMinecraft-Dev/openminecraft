#ifndef OM_NETWORK_SOCKETSTREAM_HPP
#define OM_NETWORK_SOCKETSTREAM_HPP

#include <iostream>
#include <type_traits>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>
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

    auto data() -> char *
    {
        return reinterpret_cast<char *>(buffer.data());
    }

    auto reset() -> OMNetworkPacket &
    {
        offset = 0;
        return *this;
    }

    auto readVarInt() -> uint32_t
    {
        int value = 0;

        for (int position = 0; position < 32; position += 7)
        {
            auto currentByte = read<uint8_t>();

            value |= (int)(currentByte & 0x7F) << position;

            if ((currentByte & 0x80) == 0)
            {
                return value;
            }
        }

        return value;
    }

    auto writeVarInt(int32_t value) -> OMNetworkPacket &
    {
        while ((value & ~0x7F) != 0)
        {
            buffer.push_back((value & 0x7F) | 0x80);
            value >>= 7;
        }
        buffer.push_back(value);
        return *this;
    }

    auto readUtf8WithLength() -> std::string
    {
        auto l = readVarInt();

        std::string result = {reinterpret_cast<char *>(&buffer.at(offset)), l};
        offset += l;

        return result;
    }

    auto writeUtf8WithLength(std::string s) -> OMNetworkPacket &
    {
        writeVarInt(s.size());
        for (auto ch : s)
        {
            buffer.push_back(ch);
        }
        return *this;
    }

    template <typename T> auto read() -> T
    {
        if constexpr (std::is_same_v<T, uint8_t>)
        {
            return buffer[offset++];
        }
        else
        {
            static_assert(!std::is_same_v<T, T>, "not supported!");
        }
    }

    template <typename T> auto write(T &&t) -> OMNetworkPacket &
    {
        if constexpr (std::is_same_v<T, uint8_t>)
        {
            buffer.push_back(t);
        }
        else if constexpr (std::is_same_v<T, int16_t>)
        {
            buffer.push_back((t >> 8) & 0xff);
            buffer.push_back(t & 0xff);
        }
        else if constexpr (std::is_same_v<T, int32_t>)
        {
            buffer.push_back((t >> 24) & 0xff);
            buffer.push_back((t >> 16) & 0xff);
            buffer.push_back((t >> 8) & 0xff);
            buffer.push_back(t & 0xff);
        }
        else if constexpr (std::is_same_v<T, int64_t>)
        {
            buffer.push_back((t >> 56) & 0xff);
            buffer.push_back((t >> 48) & 0xff);
            buffer.push_back((t >> 40) & 0xff);
            buffer.push_back((t >> 32) & 0xff);
            buffer.push_back((t >> 24) & 0xff);
            buffer.push_back((t >> 16) & 0xff);
            buffer.push_back((t >> 8) & 0xff);
            buffer.push_back(t & 0xff);
        }
        else
        {
            static_assert(!std::is_same_v<T, T>, "not supported!");
        }
        return *this;
    }

    auto assign(std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end)
        -> OMNetworkPacket &
    {
        buffer.assign(begin, end);
        return *this;
    }

    auto debug() -> OMNetworkPacket &
    {
        for (auto b : buffer)
        {
            std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)b << " ";
        }
        std::cout << std::dec << std::endl;

        return *this;
    }

    uint32_t id;
    auto packetId(uint32_t id) -> OMNetworkPacket &
    {
        this->id = id;
        return *this;
    }

  private:
    std::vector<uint8_t> buffer;
    uint64_t offset;
};
} // namespace openminecraft::network

#endif