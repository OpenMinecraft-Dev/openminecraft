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

    // INFO: display 里含 Text Component(NBT) 与 Slot(物品栈)，暂不支持；返回 false 表示无法继续
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
                // INFO: display 数据未实现
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
            // INFO: added components 的负载长度依赖具体组件类型，暂不支持
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

    // INFO: NBT 目前只支持读，因此不提供 write
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
