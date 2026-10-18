#ifndef OM_PROTOCOL_CODECS_HPP
#define OM_PROTOCOL_CODECS_HPP

#include "openminecraft-shell/network/om_protocol_minecraft_structs.hpp"
#include "openminecraft/network/om_network_packet.hpp"
#include "openminecraft/specs/nbt/om_nbt_reader.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

template <> struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolRegistryData>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolRegistryData
    {
        openminecraft::shell::network::OMProtocolRegistryData result;

        result.registry = pck.read<openminecraft::network::OMUtf8String>();

        auto entryCount = pck.read<openminecraft::network::OMVarInt>().value;
        result.entries.reserve(static_cast<std::size_t>(entryCount));

        for (int32_t i = 0; i < entryCount; ++i)
        {
            openminecraft::shell::network::OMProtocolRegistryData::Entry entry;
            entry.name = pck.read<openminecraft::network::OMUtf8String>();
            entry.data = pck.read<std::optional<std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>>>();

            result.entries.push_back(std::move(entry));
        }

        return result;
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolCustomReportDetails>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolCustomReportDetails
    {
        openminecraft::shell::network::OMProtocolCustomReportDetails result;

        auto detailCount = pck.read<openminecraft::network::OMVarInt>().value;
        result.details.reserve(static_cast<std::size_t>(detailCount));

        for (int32_t i = 0; i < detailCount; ++i)
        {
            openminecraft::shell::network::OMProtocolCustomReportDetails::Detail detail;
            detail.title = pck.read<openminecraft::network::OMUtf8String>();
            detail.description = pck.read<openminecraft::network::OMUtf8String>();

            result.details.push_back(std::move(detail));
        }

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolCustomReportDetails &value) -> void
    {
        pck.write(openminecraft::network::OMVarInt(static_cast<int32_t>(value.details.size())));

        for (const auto &detail : value.details)
        {
            pck.write(openminecraft::network::OMUtf8String(detail.title));
            pck.write(openminecraft::network::OMUtf8String(detail.description));
        }
    }
};

template <> struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolUpdateTags>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolUpdateTags
    {
        openminecraft::shell::network::OMProtocolUpdateTags result;

        auto groupCount = pck.read<openminecraft::network::OMVarInt>().value;
        result.groups.reserve(static_cast<std::size_t>(groupCount));

        for (int32_t g = 0; g < groupCount; ++g)
        {
            openminecraft::shell::network::OMProtocolUpdateTags::TagGroup group;
            group.registry = pck.read<openminecraft::network::OMUtf8String>();

            auto tagCount = pck.read<openminecraft::network::OMVarInt>().value;
            group.tags.reserve(static_cast<std::size_t>(tagCount));

            for (int32_t t = 0; t < tagCount; ++t)
            {
                openminecraft::shell::network::OMProtocolUpdateTags::Tag tag;
                tag.name = pck.read<openminecraft::network::OMUtf8String>();
                tag.entries = pck.readPrefixArray<openminecraft::network::OMVarInt>();

                group.tags.push_back(std::move(tag));
            }

            result.groups.push_back(std::move(group));
        }

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolUpdateTags &value) -> void
    {
        pck.write(openminecraft::network::OMVarInt(static_cast<int32_t>(value.groups.size())));

        for (const auto &group : value.groups)
        {
            pck.write(openminecraft::network::OMUtf8String(group.registry));
            pck.write(openminecraft::network::OMVarInt(static_cast<int32_t>(group.tags.size())));

            for (const auto &tag : group.tags)
            {
                pck.write(openminecraft::network::OMUtf8String(tag.name));
                pck.writePrefixArray(tag.entries);
            }
        }
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolSetDefaultSpawnPosition>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolSetDefaultSpawnPosition
    {
        openminecraft::shell::network::OMProtocolSetDefaultSpawnPosition result;

        result.dimension = pck.read<openminecraft::network::OMUtf8String>();
        result.position = pck.read<openminecraft::network::OMPackedPos>();

        result.yaw = pck.read<float>();
        result.pitch = pck.read<float>();

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolSetDefaultSpawnPosition &value) -> void
    {
        pck.write(openminecraft::network::OMUtf8String(value.dimension));
        pck.write(value.position);
        pck.write<float>(value.yaw);
        pck.write<float>(value.pitch);
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolPlayerInfoUpdate>
{
    static constexpr bool supported = true;

    enum ActionBits : uint8_t
    {
        AddPlayer = 0x01,
        InitializeChat = 0x02,
        UpdateGameMode = 0x04,
        UpdateListed = 0x08,
        UpdateLatency = 0x10,
        UpdateDisplayName = 0x20,
        UpdateHat = 0x40,
        UpdateListOrder = 0x80
    };

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolPlayerInfoUpdate
    {
        openminecraft::shell::network::OMProtocolPlayerInfoUpdate result;

        result.actions = pck.read<uint8_t>();

        auto entryCount = pck.read<openminecraft::network::OMVarInt>().value;
        result.entries.reserve(static_cast<std::size_t>(entryCount));

        for (int32_t i = 0; i < entryCount; ++i)
        {
            openminecraft::shell::network::OMProtocolPlayerInfoEntry entry;

            entry.uuid = pck.read<openminecraft::util::OMUuid>();

            if (result.actions & AddPlayer)
            {
                entry.name = pck.read<std::string>();

                auto propertyCount = pck.read<openminecraft::network::OMVarInt>().value;

                for (int32_t j = 0; j < propertyCount; ++j)
                {
                    openminecraft::shell::network::OMProtocolPlayerProperty property;
                    property.name = pck.read<std::string>();
                    property.value = pck.read<std::string>();
                    property.isSigned = pck.read<bool>();

                    if (property.isSigned)
                    {
                        property.signature = pck.read<std::string>();
                    }

                    entry.properties.push_back(std::move(property));
                }
            }

            if (result.actions & InitializeChat)
            {
                if (pck.read<bool>())
                {
                    openminecraft::shell::network::OMProtocolChatSession chat;
                    chat.sessionId = pck.read<openminecraft::util::OMUuid>();
                    chat.publicKeyExpiry = pck.read<int64_t>();
                    chat.publicKey = pck.readPrefixArray<uint8_t>();
                    chat.keySignature = pck.readPrefixArray<uint8_t>();

                    entry.chat = std::move(chat);
                }
            }

            if (result.actions & UpdateGameMode)
            {
                entry.gameMode = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
            }

            if (result.actions & UpdateListed)
            {
                entry.listed = pck.read<bool>();
            }

            if (result.actions & UpdateLatency)
            {
                entry.latency = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
            }

            if (result.actions & UpdateDisplayName)
            {
                if (pck.read<bool>())
                {
                    entry.displayName = pck.read<std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>>();
                }
            }

            if (result.actions & UpdateHat)
            {
                entry.hat = pck.read<bool>();
            }

            if (result.actions & UpdateListOrder)
            {
                entry.listOrder = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
            }

            result.entries.push_back(std::move(entry));
        }

        return result;
    }
};

// INFO: light data uses java.util.BitSet.toByteArray() masks, not long arrays:
// mask = VarInt byte count + bytes (bit i => bytes[i / 8] & (1 << (i % 8))).
namespace openminecraft::network
{
struct OMProtocolLightCodec
{
    static constexpr std::size_t arrayLength = 2048;

    static auto readMask(OMNetworkPacket &pck) -> OMBitSet
    {
        auto length = pck.read<OMVarInt>().value;

        if (length < 0 || static_cast<uint64_t>(length) > pck.restDatalen())
        {
            throw std::logic_error("[OMNetworkPacket] light mask exceeds remaining data!");
        }

        return OMBitSet::fromBytes(pck.readFixedArray<uint8_t>(static_cast<std::size_t>(length)));
    }

    static auto writeMask(OMNetworkPacket &pck, const OMBitSet &mask) -> void
    {
        auto bytes = mask.toBytes();

        pck.write(OMVarInt(static_cast<int32_t>(bytes.size())));
        pck.writeFixedArray(bytes);
    }

    static auto readArrays(OMNetworkPacket &pck, std::vector<std::vector<uint8_t>> &target) -> void
    {
        auto count = pck.read<OMVarInt>().value;

        if (count < 0 || count > 64)
        {
            throw std::logic_error("[OMNetworkPacket] light array count out of range!");
        }

        for (int32_t i = 0; i < count; ++i)
        {
            auto length = pck.read<OMVarInt>().value;

            if (length != static_cast<int32_t>(arrayLength))
            {
                throw std::logic_error("[OMNetworkPacket] light array length is not 2048!");
            }

            target.push_back(pck.readFixedArray<uint8_t>(arrayLength));
        }
    }

    static auto read(OMNetworkPacket &pck, openminecraft::shell::network::OMProtocolChunkLight &light) -> void
    {
        light.skyLightMask = readMask(pck);
        light.blockLightMask = readMask(pck);
        light.emptySkyLightMask = readMask(pck);
        light.emptyBlockLightMask = readMask(pck);

        readArrays(pck, light.skyLight);
        readArrays(pck, light.blockLight);

        if (pck.restDatalen() != 0)
        {
            // 理论上不会发生：严格解析必须恰好消费完，超出部分直接丢弃以保持流同步
            pck.readFixedArray<uint8_t>(pck.restDatalen());
        }
    }

    static auto write(OMNetworkPacket &pck, const openminecraft::shell::network::OMProtocolChunkLight &light) -> void
    {
        writeMask(pck, light.skyLightMask);
        writeMask(pck, light.blockLightMask);
        writeMask(pck, light.emptySkyLightMask);
        writeMask(pck, light.emptyBlockLightMask);

        for (const std::vector<std::vector<uint8_t>> *arrays : {&light.skyLight, &light.blockLight})
        {
            pck.write(OMVarInt(static_cast<int32_t>(arrays->size())));

            for (const auto &array : *arrays)
            {
                pck.write(OMVarInt(static_cast<int32_t>(array.size())));
                pck.writeFixedArray(array);
            }
        }
    }
};
} // namespace openminecraft::network

// INFO: the reason is a text component encoded as NBT (read only, NBT has no writer yet)
template <> struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolDisconnect>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolDisconnect
    {
        openminecraft::shell::network::OMProtocolDisconnect result;

        result.reason = pck.read<std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>>();

        return result;
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolUpdateLight>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolUpdateLight
    {
        openminecraft::shell::network::OMProtocolUpdateLight result;

        result.chunkX = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
        result.chunkZ = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());

        openminecraft::network::OMProtocolLightCodec::read(pck, result.light);

        result.complete = pck.restDatalen() == 0;

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolUpdateLight &value) -> void
    {
        pck.write(openminecraft::network::OMVarInt(value.chunkX));
        pck.write(openminecraft::network::OMVarInt(value.chunkZ));

        openminecraft::network::OMProtocolLightCodec::write(pck, value.light);
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolChunkDataAndLight>
{
    static constexpr bool supported = true;

    using Chunk = openminecraft::shell::network::OMProtocolChunkDataAndLight;
    using Section = openminecraft::shell::network::OMProtocolChunkSection;
    using Palette = openminecraft::shell::network::OMProtocolPalettedContainer;

    static constexpr std::size_t blockEntries = 4096;
    static constexpr std::size_t biomeEntries = 64;

    static auto readLongArray(openminecraft::network::OMNetworkPacket &pck) -> std::vector<int64_t>
    {
        auto count = pck.read<openminecraft::network::OMVarInt>().value;

        if (count < 0 || static_cast<uint64_t>(count) * 8 > pck.restDatalen())
        {
            throw std::logic_error("[OMNetworkPacket] long array exceeds remaining data!");
        }

        std::vector<int64_t> values;
        values.reserve(static_cast<std::size_t>(count));

        for (int32_t i = 0; i < count; ++i)
        {
            values.push_back(pck.read<int64_t>());
        }

        return values;
    }

// INFO: since 1.21.5 the data array has no length prefix; its size derives from bits and entry count
    static auto readPalettedContainer(openminecraft::network::OMNetworkPacket &pck, Palette &container,
                                      std::size_t entries, uint8_t maxIndirectBits) -> void
    {
        container.bitsPerEntry = pck.read<uint8_t>();

        if (container.bitsPerEntry == 0)
        {
            container.palette.push_back(static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>()));
            return;
        }

        if (container.bitsPerEntry <= maxIndirectBits)
        {
            auto paletteCount = pck.read<openminecraft::network::OMVarInt>().value;

            if (paletteCount < 0 || static_cast<uint64_t>(paletteCount) > pck.restDatalen())
            {
                throw std::logic_error("[OMNetworkPacket] palette exceeds remaining data!");
            }

            for (int32_t i = 0; i < paletteCount; ++i)
            {
                container.palette.push_back(static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>()));
            }
        }

        auto bits = static_cast<std::size_t>(container.bitsPerEntry);
        auto entriesPerLong = 64 / bits;
        auto longCount = (entries + entriesPerLong - 1) / entriesPerLong;

        if (longCount * 8 > pck.restDatalen())
        {
            throw std::logic_error("[OMNetworkPacket] paletted data exceeds remaining data!");
        }

        container.data.reserve(longCount);

        for (std::size_t i = 0; i < longCount; ++i)
        {
            container.data.push_back(pck.read<int64_t>());
        }
    }

// INFO: 1.16+ packing: entries never span longs and padding is appended at the end
    static auto decodeBlockStates(const Palette &container, std::vector<int32_t> &states) -> bool
    {
        if (container.bitsPerEntry == 0)
        {
            states.assign(blockEntries, container.palette.empty() ? 0 : container.palette[0]);
            return true;
        }

        auto bits = static_cast<std::size_t>(container.bitsPerEntry);
        auto entriesPerLong = 64 / bits;

        if (entriesPerLong == 0)
        {
            return false;
        }

        auto longCount = (blockEntries + entriesPerLong - 1) / entriesPerLong;

        if (container.data.size() < longCount)
        {
            return false;
        }

        const auto mask = (static_cast<uint64_t>(1) << bits) - 1;

        states.clear();
        states.reserve(blockEntries);

        for (std::size_t i = 0; i < blockEntries; ++i)
        {
            auto longIndex = i / entriesPerLong;
            auto bitIndex = (i % entriesPerLong) * bits;
            auto id = static_cast<int32_t>((static_cast<uint64_t>(container.data[longIndex]) >> bitIndex) & mask);

            if (bits <= 8)
            {
                if (id < 0 || static_cast<std::size_t>(id) >= container.palette.size())
                {
                    return false;
                }

                id = container.palette[id];
            }

            states.push_back(id);
        }

        return true;
    }

    static auto read(openminecraft::network::OMNetworkPacket &pck) -> Chunk
    {
        Chunk result;

        result.chunkX = pck.read<int32_t>();
        result.chunkZ = pck.read<int32_t>();

        auto heightmapCount = pck.read<openminecraft::network::OMVarInt>().value;

        for (int32_t i = 0; i < heightmapCount; ++i)
        {
            openminecraft::shell::network::OMProtocolHeightmap heightmap;
            heightmap.type = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
            heightmap.values = readLongArray(pck);

            result.heightmaps.push_back(std::move(heightmap));
        }

        result.dataLength = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());

        if (result.dataLength < 0 || static_cast<uint64_t>(result.dataLength) > pck.restDatalen())
        {
            throw std::logic_error("[OMNetworkPacket] chunk data exceeds remaining data!");
        }

        auto dataEnd = pck.restDatalen() - static_cast<uint64_t>(result.dataLength);

        while (pck.restDatalen() > dataEnd)
        {
            Section section;
            section.blockCount = pck.read<int16_t>();
            section.fluidCount = pck.read<int16_t>();
            readPalettedContainer(pck, section.blockStates, blockEntries, 8);
            readPalettedContainer(pck, section.biomes, biomeEntries, 3);

            result.sections.push_back(std::move(section));
        }

        if (pck.restDatalen() != dataEnd)
        {
            throw std::logic_error("[OMNetworkPacket] chunk sections overran the data blob!");
        }

        auto blockEntityCount = pck.read<openminecraft::network::OMVarInt>().value;

        for (int32_t i = 0; i < blockEntityCount; ++i)
        {
            openminecraft::shell::network::OMProtocolBlockEntity entity;
            entity.packedXZ = pck.read<uint8_t>();
            entity.y = pck.read<int16_t>();
            entity.type = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
            entity.data = pck.read<std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>>();

            result.blockEntities.push_back(std::move(entity));
        }

        openminecraft::network::OMProtocolLightCodec::read(pck, result.light);

// INFO: decode every section and compare its non-air block count with blockCount
        bool valid = !result.sections.empty();

        for (const auto &section : result.sections)
        {
            result.declaredBlockCount += section.blockCount;

            std::vector<int32_t> states;

            if (!decodeBlockStates(section.blockStates, states))
            {
                valid = false;
                continue;
            }

            int32_t nonAir = 0;

            for (auto id : states)
            {
                if (id != 0)
                {
                    ++nonAir;
                }
            }

            result.decodedBlockCount += nonAir;

            if (nonAir != section.blockCount)
            {
                valid = false;
            }
        }

        result.valid = valid;
        result.lightComplete = pck.restDatalen() == 0;

        if (!result.lightComplete)
        {
// INFO: the light array layout is still unconfirmed; record the leftover length and skip it
            result.lightTrailingBytes = static_cast<int32_t>(pck.restDatalen());
            pck.readFixedArray<uint8_t>(pck.restDatalen());
        }

        return result;
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolUpdateEntityPosition>
{
    static constexpr bool supported = true;

    // INFO: 相对位移在线上是 Short，单位 1/4096 格
    static auto readDelta(openminecraft::network::OMNetworkPacket &pck) -> double
    {
        return static_cast<double>(pck.read<int16_t>()) / openminecraft::shell::network::OMProtocolUpdateEntityPosition::deltaUnitsPerBlock;
    }

    static auto writeDelta(openminecraft::network::OMNetworkPacket &pck, double value) -> void
    {
        auto raw = std::lround(value * openminecraft::shell::network::OMProtocolUpdateEntityPosition::deltaUnitsPerBlock);
        raw = std::clamp<long>(raw, -32768, 32767);

        pck.write<int16_t>(static_cast<int16_t>(raw));
    }

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolUpdateEntityPosition
    {
        openminecraft::shell::network::OMProtocolUpdateEntityPosition result;

        result.entityId = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
        result.deltaX = readDelta(pck);
        result.deltaY = readDelta(pck);
        result.deltaZ = readDelta(pck);
        result.onGround = pck.read<bool>();

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolUpdateEntityPosition &value) -> void
    {
        pck.write(openminecraft::network::OMVarInt(value.entityId));
        writeDelta(pck, value.deltaX);
        writeDelta(pck, value.deltaY);
        writeDelta(pck, value.deltaZ);
        pck.write<bool>(value.onGround);
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<
    openminecraft::shell::network::OMProtocolUpdateEntityPositionRotation>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolUpdateEntityPositionRotation
    {
        openminecraft::shell::network::OMProtocolUpdateEntityPositionRotation result;

        result.entityId = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
        result.deltaX = static_cast<double>(pck.read<int16_t>()) /
                        openminecraft::shell::network::OMProtocolUpdateEntityPosition::deltaUnitsPerBlock;
        result.deltaY = static_cast<double>(pck.read<int16_t>()) /
                        openminecraft::shell::network::OMProtocolUpdateEntityPosition::deltaUnitsPerBlock;
        result.deltaZ = static_cast<double>(pck.read<int16_t>()) /
                        openminecraft::shell::network::OMProtocolUpdateEntityPosition::deltaUnitsPerBlock;
        result.yaw = pck.read<openminecraft::network::OMAngle>();
        result.pitch = pck.read<openminecraft::network::OMAngle>();
        result.onGround = pck.read<bool>();

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolUpdateEntityPositionRotation &value) -> void
    {
        using Position = openminecraft::network::OMNetworkPacketCodec<
            openminecraft::shell::network::OMProtocolUpdateEntityPosition>;

        pck.write(openminecraft::network::OMVarInt(value.entityId));
        Position::writeDelta(pck, value.deltaX);
        Position::writeDelta(pck, value.deltaY);
        Position::writeDelta(pck, value.deltaZ);
        pck.write(value.yaw);
        pck.write(value.pitch);
        pck.write<bool>(value.onGround);
    }
};

template <> struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolSpawnEntity>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolSpawnEntity
    {
        openminecraft::shell::network::OMProtocolSpawnEntity result;

        result.entityId = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
        result.uuid = pck.read<openminecraft::util::OMUuid>();
        result.type = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
        result.x = pck.read<double>();
        result.y = pck.read<double>();
        result.z = pck.read<double>();
        result.velocity = pck.read<openminecraft::network::OMLpVec3>();
        result.pitch = pck.read<openminecraft::network::OMAngle>();
        result.yaw = pck.read<openminecraft::network::OMAngle>();
        result.headYaw = pck.read<openminecraft::network::OMAngle>();
        result.data = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolSpawnEntity &value) -> void
    {
        pck.write(openminecraft::network::OMVarInt(value.entityId));
        pck.write(value.uuid);
        pck.write(openminecraft::network::OMVarInt(value.type));
        pck.write<double>(value.x);
        pck.write<double>(value.y);
        pck.write<double>(value.z);
        pck.write(value.velocity);
        pck.write(value.pitch);
        pck.write(value.yaw);
        pck.write(value.headYaw);
        pck.write(openminecraft::network::OMVarInt(value.data));
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolRemoveEntities>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolRemoveEntities
    {
        openminecraft::shell::network::OMProtocolRemoveEntities result;

        auto count = pck.read<openminecraft::network::OMVarInt>().value;

        if (count < 0 || static_cast<uint64_t>(count) > pck.restDatalen())
        {
            throw std::logic_error("[OMNetworkPacket] entity id array exceeds remaining data!");
        }

        result.entityIds.reserve(static_cast<std::size_t>(count));

        for (int32_t i = 0; i < count; ++i)
        {
            result.entityIds.push_back(static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>()));
        }

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolRemoveEntities &value) -> void
    {
        pck.write(openminecraft::network::OMVarInt(static_cast<int32_t>(value.entityIds.size())));

        for (auto id : value.entityIds)
        {
            pck.write(openminecraft::network::OMVarInt(id));
        }
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolSetHeadRotation>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolSetHeadRotation
    {
        openminecraft::shell::network::OMProtocolSetHeadRotation result;

        result.entityId = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
        result.headYaw = pck.read<openminecraft::network::OMAngle>();

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolSetHeadRotation &value) -> void
    {
        pck.write(openminecraft::network::OMVarInt(value.entityId));
        pck.write(value.headYaw);
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolTeleportEntity>
{
    static constexpr bool supported = true;

    enum PathType : int32_t
    {
        Linear = 0,
        Stepped = 1
    };

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolTeleportEntity
    {
        openminecraft::shell::network::OMProtocolTeleportEntity result;

        result.entityId = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
        result.pathType = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());

        if (result.pathType == Linear)
        {
            result.endX = pck.read<double>();
            result.endY = pck.read<double>();
            result.endZ = pck.read<double>();
        }
        else
        {
            auto stepCount = pck.read<openminecraft::network::OMVarInt>().value;

            if (stepCount < 0 || static_cast<uint64_t>(stepCount) > pck.restDatalen())
            {
                throw std::logic_error("[OMNetworkPacket] teleport steps exceed remaining data!");
            }

            for (int32_t i = 0; i < stepCount; ++i)
            {
                openminecraft::shell::network::OMProtocolPositionStep step;
                step.x = pck.read<double>();
                step.y = pck.read<double>();
                step.z = pck.read<double>();
                step.tickOffset = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());

                result.steps.push_back(std::move(step));
            }
        }

        result.yaw = pck.read<float>();
        result.pitch = pck.read<float>();
        result.onGround = pck.read<bool>();

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolTeleportEntity &value) -> void
    {
        pck.write(openminecraft::network::OMVarInt(value.entityId));
        pck.write(openminecraft::network::OMVarInt(value.pathType));

        if (value.pathType == Linear)
        {
            pck.write<double>(value.endX);
            pck.write<double>(value.endY);
            pck.write<double>(value.endZ);
        }
        else
        {
            pck.write(openminecraft::network::OMVarInt(static_cast<int32_t>(value.steps.size())));

            for (const auto &step : value.steps)
            {
                pck.write<double>(step.x);
                pck.write<double>(step.y);
                pck.write<double>(step.z);
                pck.write(openminecraft::network::OMVarInt(step.tickOffset));
            }
        }

        pck.write<float>(value.yaw);
        pck.write<float>(value.pitch);
        pck.write<bool>(value.onGround);
    }
};

template <> struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolSetExperience>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolSetExperience
    {
        openminecraft::shell::network::OMProtocolSetExperience result;

        result.experienceBar = pck.read<float>();
        result.level = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
        result.totalExperience = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolSetExperience &value) -> void
    {
        pck.write<float>(value.experienceBar);
        pck.write(openminecraft::network::OMVarInt(value.level));
        pck.write(openminecraft::network::OMVarInt(value.totalExperience));
    }
};

template <> struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolSetHealth>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolSetHealth
    {
        openminecraft::shell::network::OMProtocolSetHealth result;

        result.health = pck.read<float>();
        result.food = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
        result.foodSaturation = pck.read<float>();

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolSetHealth &value) -> void
    {
        pck.write<float>(value.health);
        pck.write(openminecraft::network::OMVarInt(value.food));
        pck.write<float>(value.foodSaturation);
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolUpdateAdvancements>
{
    static constexpr bool supported = true;

// INFO: display data holds a Text Component (NBT) and a Slot (item stack); not supported yet
    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolUpdateAdvancements
    {
        openminecraft::shell::network::OMProtocolUpdateAdvancements result;

        result.reset = pck.read<bool>();

        auto addedCount = pck.read<openminecraft::network::OMVarInt>().value;

        for (int32_t i = 0; i < addedCount; ++i)
        {
            openminecraft::shell::network::OMProtocolAdvancement advancement;
            advancement.id = pck.read<std::string>();

            if (pck.read<bool>())
            {
                advancement.parent = pck.read<std::string>();
            }

            if (pck.read<bool>())
            {
// INFO: display data is not implemented yet
                result.added.push_back(std::move(advancement));
                result.truncated = true;
                pck.readFixedArray<uint8_t>(pck.restDatalen());

                return result;
            }

            auto requirementCount = pck.read<openminecraft::network::OMVarInt>().value;

            for (int32_t j = 0; j < requirementCount; ++j)
            {
                std::vector<std::string> group;
                auto criteriaCount = pck.read<openminecraft::network::OMVarInt>().value;

                for (int32_t k = 0; k < criteriaCount; ++k)
                {
                    group.push_back(pck.read<std::string>());
                }

                advancement.requirements.push_back(std::move(group));
            }

            advancement.sendsTelemetry = pck.read<bool>();
            advancement.unknownTail = pck.read<int64_t>();

            result.added.push_back(std::move(advancement));
        }

        auto removedCount = pck.read<openminecraft::network::OMVarInt>().value;

        for (int32_t i = 0; i < removedCount; ++i)
        {
            result.removed.push_back(pck.read<std::string>());
        }

        auto progressCount = pck.read<openminecraft::network::OMVarInt>().value;

        for (int32_t i = 0; i < progressCount; ++i)
        {
            openminecraft::shell::network::OMProtocolAdvancementProgress progress;
            progress.id = pck.read<std::string>();

            auto criteriaCount = pck.read<openminecraft::network::OMVarInt>().value;

            for (int32_t j = 0; j < criteriaCount; ++j)
            {
                openminecraft::shell::network::OMProtocolAdvancementCriterion criterion;
                criterion.id = pck.read<std::string>();

                if (pck.read<bool>())
                {
                    criterion.date = pck.read<int64_t>();
                }

                progress.criteria.push_back(std::move(criterion));
            }

            result.progress.push_back(std::move(progress));
        }

        result.showAdvancements = pck.read<bool>();

        return result;
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolUpdateAttributes>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolUpdateAttributes
    {
        openminecraft::shell::network::OMProtocolUpdateAttributes result;

        result.entityId = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());

        auto attributeCount = pck.read<openminecraft::network::OMVarInt>().value;
        result.attributes.reserve(static_cast<std::size_t>(attributeCount));

        for (int32_t i = 0; i < attributeCount; ++i)
        {
            openminecraft::shell::network::OMProtocolAttribute attribute;
            attribute.attributeId = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
            attribute.base = pck.read<double>();

            auto modifierCount = pck.read<openminecraft::network::OMVarInt>().value;

            for (int32_t j = 0; j < modifierCount; ++j)
            {
                openminecraft::shell::network::OMProtocolAttributeModifier modifier;
                modifier.id = pck.read<std::string>();
                modifier.amount = pck.read<double>();
                modifier.operation = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());

                attribute.modifiers.push_back(std::move(modifier));
            }

            result.attributes.push_back(std::move(attribute));
        }

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolUpdateAttributes &value) -> void
    {
        pck.write(openminecraft::network::OMVarInt(value.entityId));
        pck.write(openminecraft::network::OMVarInt(static_cast<int32_t>(value.attributes.size())));

        for (const auto &attribute : value.attributes)
        {
            pck.write(openminecraft::network::OMVarInt(attribute.attributeId));
            pck.write<double>(attribute.base);
            pck.write(openminecraft::network::OMVarInt(static_cast<int32_t>(attribute.modifiers.size())));

            for (const auto &modifier : attribute.modifiers)
            {
                pck.write(openminecraft::network::OMUtf8String(modifier.id));
                pck.write<double>(modifier.amount);
                pck.write(openminecraft::network::OMVarInt(modifier.operation));
            }
        }
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolSetContainerContent>
{
    static constexpr bool supported = true;

    static auto readSlot(openminecraft::network::OMNetworkPacket &pck,
                         openminecraft::shell::network::OMProtocolItemStack &slot) -> bool
    {
        slot.count = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());

        if (slot.count <= 0)
        {
            return true;
        }

        slot.itemId = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());

        auto addedCount = pck.read<openminecraft::network::OMVarInt>().value;

        if (addedCount > 0)
        {
// INFO: the payload length of added components depends on the component type; not supported yet
            return false;
        }

        auto removedCount = pck.read<openminecraft::network::OMVarInt>().value;

        for (int32_t i = 0; i < removedCount; ++i)
        {
            slot.removedComponents.push_back(
                static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>()));
        }

        return true;
    }

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolSetContainerContent
    {
        openminecraft::shell::network::OMProtocolSetContainerContent result;

        result.windowId = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());
        result.stateId = static_cast<int32_t>(pck.read<openminecraft::network::OMVarInt>());

        auto slotCount = pck.read<openminecraft::network::OMVarInt>().value;
        result.slots.reserve(static_cast<std::size_t>(slotCount));

        for (int32_t i = 0; i < slotCount; ++i)
        {
            openminecraft::shell::network::OMProtocolItemStack slot;

            if (!readSlot(pck, slot))
            {
                result.slots.push_back(std::move(slot));
                result.truncated = true;
                pck.readFixedArray<uint8_t>(pck.restDatalen());

                return result;
            }

            result.slots.push_back(std::move(slot));
        }

        if (!readSlot(pck, result.carriedItem))
        {
            result.truncated = true;
            pck.readFixedArray<uint8_t>(pck.restDatalen());
        }

        return result;
    }
};

template <> struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolPostEffects>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolPostEffects
    {
        openminecraft::shell::network::OMProtocolPostEffects result;
        result.effects = pck.readPrefixArray<std::string>();

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolPostEffects &value) -> void
    {
        pck.writePrefixArray(value.effects);
    }
};

template <> struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolServerData>
{
    static constexpr bool supported = true;

// INFO: NBT is read-only for now, so no write is provided
    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolServerData
    {
        openminecraft::shell::network::OMProtocolServerData result;

        result.motd = pck.read<std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>>();

        if (pck.read<bool>())
        {
            result.icon = pck.readPrefixArray<uint8_t>();
        }

        return result;
    }
};

template <> struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolSetTime>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolSetTime
    {
        openminecraft::shell::network::OMProtocolSetTime result;

        result.gameTime = pck.read<int64_t>();

        auto clockCount = pck.read<openminecraft::network::OMVarInt>().value;
        result.clocks.reserve(static_cast<std::size_t>(clockCount));

        for (int32_t i = 0; i < clockCount; ++i)
        {
            openminecraft::shell::network::OMProtocolSetTime::Clock clock;
            clock.id = pck.read<openminecraft::network::OMVarInt>();
            clock.totalTicks = pck.read<openminecraft::network::OMVarLong>();
            clock.partialTick = pck.read<float>();
            clock.rate = pck.read<float>();

            result.clocks.push_back(clock);
        }

        return result;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolSetTime &value) -> void
    {
        pck.write<int64_t>(value.gameTime).write(openminecraft::network::OMVarInt(static_cast<int32_t>(value.clocks.size())));

        for (const auto &clock : value.clocks)
        {
            pck.write(openminecraft::network::OMVarInt(clock.id));
            pck.write(openminecraft::network::OMVarLong(clock.totalTicks));
            pck.write<float>(clock.partialTick);
            pck.write<float>(clock.rate);
        }
    }
};

template <> struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolKnownPack>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck) -> openminecraft::shell::network::OMProtocolKnownPack
    {
        return {pck.read<openminecraft::network::OMUtf8String>(), pck.read<openminecraft::network::OMUtf8String>(),
                pck.read<openminecraft::network::OMUtf8String>()};
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolKnownPack &value) -> void
    {
        pck.write(openminecraft::network::OMUtf8String(value.namesp));
        pck.write(openminecraft::network::OMUtf8String(value.id));
        pck.write(openminecraft::network::OMUtf8String(value.version));
    }
};

template <> struct openminecraft::network::OMNetworkPacketCodec<std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> std::shared_ptr<openminecraft::specs::nbt::OMNbtTag>
    {
        openminecraft::specs::nbt::OMNbtReader reader(pck.asStream());

        return reader.parse(false);
    }
};

template <>
struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolGameProfileProperty>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolGameProfileProperty
    {
        return {pck.read<openminecraft::network::OMUtf8String>(), pck.read<openminecraft::network::OMUtf8String>(),
                pck.read<openminecraft::network::OMUtf8String>()};
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolGameProfileProperty &value) -> void
    {
        pck.write(openminecraft::network::OMUtf8String(value.name));
        pck.write(openminecraft::network::OMUtf8String(value.value));
        pck.write(openminecraft::network::OMUtf8String(value.signature));
    }
};

template <> struct openminecraft::network::OMNetworkPacketCodec<openminecraft::shell::network::OMProtocolGameProfile>
{
    static constexpr bool supported = true;

    static auto read(openminecraft::network::OMNetworkPacket &pck)
        -> openminecraft::shell::network::OMProtocolGameProfile
    {
        openminecraft::shell::network::OMProtocolGameProfile profile;

        profile.uuid = pck.read<openminecraft::util::OMUuid>();
        profile.name = pck.read<openminecraft::network::OMUtf8String>();
        profile.properties = pck.readPrefixArray<openminecraft::shell::network::OMProtocolGameProfileProperty>();
        profile.sessionId = pck.read<openminecraft::util::OMUuid>();

        return profile;
    }

    static auto write(openminecraft::network::OMNetworkPacket &pck,
                      const openminecraft::shell::network::OMProtocolGameProfile &profile) -> void
    {
        pck.write(profile.uuid);
        pck.write(openminecraft::network::OMUtf8String(profile.name));
        pck.writePrefixArray(profile.properties);
        pck.write(profile.sessionId);
    }
};

#endif
