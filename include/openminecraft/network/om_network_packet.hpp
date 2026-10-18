#ifndef OM_NETWORK_PACKET_HPP
#define OM_NETWORK_PACKET_HPP

#include "openminecraft/util/om_util_uuid.hpp"
#include <iostream>
#include <istream>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <fmt/format.h>
namespace openminecraft::network
{
struct OMVarInt
{
    int32_t value = 0;

    constexpr OMVarInt() = default;
    constexpr OMVarInt(int32_t v) : value(v)
    {
    }

    constexpr operator int32_t() const
    {
        return value;
    }
};

struct OMVarLong
{
    int64_t value = 0;

    constexpr OMVarLong() = default;
    constexpr OMVarLong(int64_t v) : value(v)
    {
    }

    constexpr operator int64_t() const
    {
        return value;
    }
};

// INFO: Minecraft 协议的 Position：x/z 各 26 位、y 12 位（补码）打包进一个 Long。
struct OMPackedPos
{
    int64_t value = 0;

    constexpr OMPackedPos() = default;
    constexpr explicit OMPackedPos(int64_t packed) : value(packed)
    {
    }

    static constexpr auto from(int32_t x, int32_t y, int32_t z) -> OMPackedPos
    {
        return OMPackedPos(static_cast<int64_t>((static_cast<uint64_t>(x) & 0x3FFFFFF) << 38 |
                                                (static_cast<uint64_t>(z) & 0x3FFFFFF) << 12 |
                                                (static_cast<uint64_t>(y) & 0xFFF)));
    }

    constexpr auto x() const -> int32_t
    {
        return signExtend(static_cast<uint64_t>(value) >> 38, 26);
    }

    constexpr auto y() const -> int32_t
    {
        return signExtend(static_cast<uint64_t>(value) & 0xFFF, 12);
    }

    constexpr auto z() const -> int32_t
    {
        return signExtend((static_cast<uint64_t>(value) >> 12) & 0x3FFFFFF, 26);
    }

  private:
    static constexpr auto signExtend(uint64_t raw, int bits) -> int32_t
    {
        auto scoped = static_cast<int64_t>(raw);

        if (scoped >= (static_cast<int64_t>(1) << (bits - 1)))
        {
            scoped -= static_cast<int64_t>(1) << bits;
        }

        return static_cast<int32_t>(scoped);
    }
};

struct OMUtf8String
{
    std::string value;

    OMUtf8String() = default;
    OMUtf8String(std::string v) : value(std::move(v))
    {
    }

    operator const std::string &() const
    {
        return value;
    }
};

template <typename T> struct OMNetworkPacketCodec
{
    static constexpr bool supported = false;
};

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

    template <typename T> auto readPrefixArray() -> std::vector<T>
    {
        auto count = read<OMVarInt>().value;

        if (count <= 0)
        {
            return {};
        }

        if (static_cast<uint64_t>(count) > restDatalen())
        {
            throw std::logic_error("[OMNetworkPacket] prefix array length exceeds remaining data!");
        }

        auto length = static_cast<std::size_t>(count);

        if constexpr (std::is_same_v<T, uint8_t>)
        {
            std::vector<T> data(length);

            std::memcpy(data.data(), buffer.data() + offset, length);
            offset += length;

            return data;
        }
        else
        {
            std::vector<T> data;
            data.reserve(length);

            for (std::size_t i = 0; i < length; ++i)
            {
                data.push_back(read<T>());
            }

            return data;
        }
    }

    template <typename T> auto readFixedArray(std::size_t count) -> std::vector<T>
    {
        if (count == 0)
        {
            return {};
        }

        if (static_cast<uint64_t>(count) > restDatalen())
        {
            throw std::logic_error("[OMNetworkPacket] fixed array length exceeds remaining data!");
        }

        if constexpr (std::is_same_v<T, uint8_t>)
        {
            std::vector<T> data(count);

            std::memcpy(data.data(), buffer.data() + offset, count);
            offset += count;

            return data;
        }
        else
        {
            std::vector<T> data;
            data.reserve(count);

            for (std::size_t i = 0; i < count; ++i)
            {
                data.push_back(read<T>());
            }

            return data;
        }
    }

    template <typename T> auto read() -> T
    {
        if constexpr (std::is_same_v<T, OMVarInt>)
        {
            uint32_t value = 0;

            for (int position = 0; position < 32; position += 7)
            {
                auto currentByte = read<uint8_t>();

                value |= static_cast<uint32_t>(currentByte & 0x7F) << position;

                if ((currentByte & 0x80) == 0)
                {
                    break;
                }
            }

            return OMVarInt(static_cast<int32_t>(value));
        }
        else if constexpr (std::is_same_v<T, OMVarLong>)
        {
            uint64_t value = 0;

            for (int position = 0; position < 64; position += 7)
            {
                auto currentByte = read<uint8_t>();

                value |= static_cast<uint64_t>(currentByte & 0x7F) << position;

                if ((currentByte & 0x80) == 0)
                {
                    break;
                }
            }

            return OMVarLong(static_cast<int64_t>(value));
        }
        else if constexpr (std::is_same_v<T, OMPackedPos>)
        {
            return OMPackedPos(read<int64_t>());
        }
        else if constexpr (std::is_same_v<T, OMUtf8String> || std::is_same_v<T, std::string>)
        {
            auto length = read<OMVarInt>().value;

            if (length <= 0)
            {
                return {};
            }

            if (static_cast<uint64_t>(length) > restDatalen())
            {
                throw std::logic_error("[OMNetworkPacket] string length exceeds remaining data!");
            }

            std::string value = {reinterpret_cast<char *>(&buffer.at(offset)), static_cast<std::size_t>(length)};
            offset += length;

            return value;
        }
        else if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, bool>)
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
        else if constexpr (std::is_same_v<T, float>)
        {
            auto l = read<int32_t>();

            return *reinterpret_cast<float *>(&l);
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
        else if constexpr (std::is_same_v<T, double>)
        {
            auto l = read<int64_t>();

            return *reinterpret_cast<double *>(&l);
        }
        else if constexpr (std::is_same_v<T, util::OMUuid>)
        {
            auto high = read<int64_t>();
            auto low = read<int64_t>();

            return util::OMUuid(static_cast<uint64_t>(low), static_cast<uint64_t>(high));
        }
        else if constexpr (OMNetworkPacketCodec<T>::supported)
        {
            return OMNetworkPacketCodec<T>::read(*this);
        }
        else
        {
            static_assert(!std::is_same_v<T, T>,
                          "OMNetworkPacket::read: unsupported type, specialize OMNetworkPacketCodec<T> to extend");
        }
    }

    template <typename T> auto write(T t) -> OMNetworkPacket &
    {
        if constexpr (std::is_same_v<T, OMVarInt>)
        {
            auto value = static_cast<uint32_t>(t.value);

            while ((value & ~0x7Fu) != 0)
            {
                buffer.push_back(static_cast<uint8_t>((value & 0x7F) | 0x80));
                value >>= 7;
            }

            buffer.push_back(static_cast<uint8_t>(value));
        }
        else if constexpr (std::is_same_v<T, OMVarLong>)
        {
            auto value = static_cast<uint64_t>(t.value);

            while ((value & ~0x7Full) != 0)
            {
                buffer.push_back(static_cast<uint8_t>((value & 0x7F) | 0x80));
                value >>= 7;
            }

            buffer.push_back(static_cast<uint8_t>(value));
        }
        else if constexpr (std::is_same_v<T, OMPackedPos>)
        {
            write<int64_t>(t.value);
        }
        else if constexpr (std::is_same_v<T, OMUtf8String> || std::is_same_v<T, std::string>)
        {
            const std::string &value = t;

            write(OMVarInt(static_cast<int32_t>(value.size())));
            buffer.insert(buffer.end(), value.begin(), value.end());
        }
        else if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, bool>)
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
        else if constexpr (std::is_same_v<T, float>)
        {
            write<int32_t>(*reinterpret_cast<int32_t *>(&t));
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            write<int64_t>(*reinterpret_cast<int64_t *>(&t));
        }
        else if constexpr (std::is_same_v<T, util::OMUuid>)
        {
            write<int64_t>(t.highBits());
            write<int64_t>(t.lowBits());
        }
        else if constexpr (OMNetworkPacketCodec<T>::supported)
        {
            OMNetworkPacketCodec<T>::write(*this, t);
        }
        else
        {
            static_assert(!std::is_same_v<T, T>,
                          "OMNetworkPacket::write: unsupported type, specialize OMNetworkPacketCodec<T> to extend");
        }
        return *this;
    }

    template <typename T> auto writeFixedArray(const std::vector<T> &values) -> OMNetworkPacket &
    {
        if constexpr (std::is_same_v<T, uint8_t>)
        {
            buffer.insert(buffer.end(), values.begin(), values.end());
        }
        else
        {
            for (const auto &value : values)
            {
                write<T>(value);
            }
        }

        return *this;
    }

    template <typename T> auto writePrefixArray(const std::vector<T> &values) -> OMNetworkPacket &
    {
        write(OMVarInt(static_cast<int32_t>(values.size())));

        return writeFixedArray(values);
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

    auto debug(int l = 24, bool o = true, int customoff = 0) -> OMNetworkPacket &
    {
        int skip = o ? offset : 0;
        skip += customoff;
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

template <typename T> struct OMNetworkPacketCodec<std::optional<T>>
{
    static constexpr bool supported = true;

    static auto read(OMNetworkPacket &pck) -> std::optional<T>
    {
        if (!pck.read<bool>())
        {
            return std::nullopt;
        }

        return pck.read<T>();
    }

    static auto write(OMNetworkPacket &pck, const std::optional<T> &value) -> void
    {
        pck.write<bool>(value.has_value());

        if (value.has_value())
        {
            pck.write<T>(*value);
        }
    }
};

inline auto toStdStrings(const std::vector<OMUtf8String> &values) -> std::vector<std::string>
{
    std::vector<std::string> result;
    result.reserve(values.size());

    for (const auto &value : values)
    {
        result.push_back(value.value);
    }

    return result;
}

inline auto toUtf8Strings(const std::vector<std::string> &values) -> std::vector<OMUtf8String>
{
    std::vector<OMUtf8String> result;
    result.reserve(values.size());

    for (const auto &value : values)
    {
        result.emplace_back(value);
    }

    return result;
}
} // namespace openminecraft::network

template <> struct fmt::formatter<openminecraft::network::OMVarInt> : formatter<int32_t>
{
    auto format(openminecraft::network::OMVarInt v, format_context &ctx) const -> format_context::iterator
    {
        return formatter<int32_t>::format(v.value, ctx);
    }
};

template <> struct fmt::formatter<openminecraft::network::OMVarLong> : formatter<int64_t>
{
    auto format(openminecraft::network::OMVarLong v, format_context &ctx) const -> format_context::iterator
    {
        return formatter<int64_t>::format(v.value, ctx);
    }
};

template <> struct fmt::formatter<openminecraft::network::OMUtf8String> : formatter<string_view>
{
    auto format(const openminecraft::network::OMUtf8String &v, format_context &ctx) const -> format_context::iterator
    {
        return formatter<string_view>::format(v.value, ctx);
    }
};

#endif