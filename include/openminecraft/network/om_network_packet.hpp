#ifndef OM_NETWORK_SOCKETSTREAM_HPP
#define OM_NETWORK_SOCKETSTREAM_HPP

#include "openminecraft/util/om_util_uuid.hpp"
#include <iostream>
#include <istream>
#include <memory>
#include <type_traits>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>
#include <cstring>
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

    auto restDatalen() -> uint64_t
    {
        return buffer.size() - offset;
    }

    auto restData() -> char *
    {
        return reinterpret_cast<char *>(buffer.data()) + offset;
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

    template <typename T> auto readPrefixArray() -> std::vector<T>
    {
        auto length = readVarInt();
        if constexpr (std::is_same_v<T, uint8_t>)
        {
            std::vector<T> data;
            data.resize(length);

            std::memcpy(data.data(), buffer.data() + offset, length);
            offset += length;

            return data;
        }
        else
        {
            static_assert(!std::is_same_v<T, T>, "not supported!");
        }
    }

    template <typename T> auto read() -> T
    {
        if constexpr (std::is_same_v<T, uint8_t>)
        {
            return buffer[offset++];
        }
        else if constexpr (std::is_same_v<T, int16_t>)
        {
            int16_t d = 0;
            for (int i = 0; i < 2; ++i)
            {
                d <<= 8;
                d |= buffer[offset++];
            }
            return d;
        }
        else if constexpr (std::is_same_v<T, int32_t>)
        {
            int32_t d = 0;
            for (int i = 0; i < 4; ++i)
            {
                d <<= 8;
                d |= buffer[offset++];
            }
            return d;
        }
        else if constexpr (std::is_same_v<T, int64_t>)
        {
            int64_t d = 0;
            for (int i = 0; i < 8; ++i)
            {
                d <<= 8;
                d |= buffer[offset++];
            }
            return d;
        }
        else if constexpr (std::is_same_v<T, util::OMUuid>)
        {
            return util::OMUuid(read<int64_t>(), read<int64_t>());
        }
        else
        {
            static_assert(!std::is_same_v<T, T>, "not supported!");
        }
    }

    template <typename T> auto write(T t) -> OMNetworkPacket &
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
        else if constexpr (std::is_same_v<T, util::OMUuid>)
        {
            write<int64_t>(t.highBits());
            write<int64_t>(t.lowBits());
        }
        else
        {
            static_assert(!std::is_same_v<T, T>, "not supported!");
        }
        return *this;
    }

    auto readRaw(uint8_t *buf, uint64_t l) -> uint64_t
    {
        *buf = buffer[offset++];
        return 1;
    }

    auto assign(std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end)
        -> OMNetworkPacket &
    {
        buffer.assign(begin, end);
        return *this;
    }

    auto debug(int l = 24) -> OMNetworkPacket &
    {
        int skip = offset;
        int i = 0;
        for (auto b : buffer)
        {
            if (skip > 0)
            {
                skip--;
                continue;
            }

            std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)b << " ";
            if (i > l)
            {
                std::cout << " ...";
                break;
            }
            i++;
        }
        if (i == 0)
        {
            std::cout << "(empty packet)";
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

    auto asStream() -> std::shared_ptr<std::istream>;

  private:
    std::vector<uint8_t> buffer;
    uint64_t offset;

    friend class OMNetworkPacketStreamBuf;
};
} // namespace openminecraft::network

#endif