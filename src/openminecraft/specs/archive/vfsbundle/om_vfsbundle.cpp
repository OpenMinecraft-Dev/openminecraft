#include "openminecraft/specs/vfsbundle/om_vfsbundle.hpp"

#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"

#include <iostream>
#include <memory>

namespace openminecraft::specs::vfsbundle
{
using std::string;

auto operator!=(const std::array<char, 6> &hd, const std::array<uint8_t, 6> &rhs) -> bool
{
    for (int i = 0; i < 6; i++)
    {
        if (hd[i] != rhs[i])
        {
            return true;
        }
    }

    return false;
}

constexpr auto rdbe64 = [](std::shared_ptr<std::istream> stream) -> uint64_t {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++)
    {
        uint8_t bt = 0;
        stream->read(reinterpret_cast<char *>(&bt), 1);

        result <<= 8;
        result |= bt;
    }

    return result;
};
constexpr auto rdbe64p = [](char *&p) -> uint64_t {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++)
    {
        uint8_t bt = *p;
        p++;

        result <<= 8;
        result |= bt;
    }

    return result;
};

constexpr auto rdstring = [](std::shared_ptr<std::istream> stream) -> std::string {
    std::vector<char> chs;
    while (true)
    {
        char c;
        stream->read(&c, 1);
        if (c == '\0')
        {
            break;
        }

        chs.push_back(c);
    }

    return string(chs.begin(), chs.end());
};

OMBundle::OMBundle() : logger("OMBundle", this)
{
    writable = true;
}

OMBundle::OMBundle(std::shared_ptr<std::istream> stream) : OMBundle()
{
    std::array<char, 6> hd = {};
    stream->read(hd.data(), 6);

#define STRERR                                                                                                         \
    if (stream->bad())                                                                                                 \
    {                                                                                                                  \
        throw std::runtime_error("Bad stream!");                                                                       \
    }

    STRERR
    if (hd != header)
    {
        throw std::runtime_error("Unexpected header!");
    }

    stream->read(hd.data(), 2);

    auto nentry = rdbe64(stream);
    STRERR

    for (uint64_t i = 0; i < nentry; i++)
    {
        auto off = rdbe64(stream);
        auto fileoff = rdbe64(stream);
        auto pp = stream->tellg();
        STRERR

        stream->seekg(static_cast<std::streamoff>(off));
        auto fmd = fetchMetadata(stream);
        STRERR

        stream->seekg(static_cast<std::streamoff>(fileoff));

        auto dd = new uint8_t[fmd.length];
        stream->read(reinterpret_cast<char *>(dd), static_cast<std::streamsize>(fmd.length));
        STRERR

        files.emplace_back(fmd, dd);

        stream->seekg(pp);
    }

    writable = true;
}

OMBundle::OMBundle(void *base, uint64_t length) : OMBundle()
{
    if (std::strcmp(static_cast<char *>(base), "OMVFS") != 0)
    {
        throw std::runtime_error("Unexpected header!");
    }

    auto current = static_cast<char *>(base) + 8;

#define PTRERR                                                                                                         \
    if (current - static_cast<char *>(base) >= length)                                                                 \
    {                                                                                                                  \
        throw std::runtime_error("Buffer overflow!");                                                                  \
    }

    PTRERR

    auto nentry = rdbe64p(current);
    PTRERR

    for (uint64_t i = 0; i < nentry; i++)
    {
        // gino: off1 for file metadata, off2 for actual content pointer
        auto off1 = rdbe64p(current);
        auto off2 = rdbe64p(current);

        auto entBegin = static_cast<char *>(base) + off1;

        auto timestamp = rdbe64p(entBegin);
        auto length = rdbe64p(entBegin);
        auto filename = std::string(entBegin);
        entBegin += std::strlen(entBegin) + 1; // bump 1 byte for next string!
        auto owner = std::string(entBegin);

        files.emplace_back(OMBundleFileMetadata{timestamp, length, filename, owner},
                           static_cast<uint8_t *>(base) + off2);
    }

    writable = false;
}

OMBundle::~OMBundle()
{
    if (writable)
    {
        for (auto itt : files)
        {
            mem::allocator::tracedFreeSpecs(itt.second);
        }
    }
}

// gino: we don't know how long the file is!
void OMBundle::appendFile(OMBundleFileMetadata metadata, std::istream &stream)
{
    for (auto &ch : metadata.name)
    {
        if (ch == '\\')
        {
            ch = '/';
        }
    }

    stream.seekg(0, std::ios::end);
    auto length = stream.tellg();
    metadata.length = length;
    stream.seekg(0, std::ios::beg);
    auto result = reinterpret_cast<uint8_t *>(mem::allocator::tracedCallocSpecs(1, metadata.length));
    stream.read(reinterpret_cast<char *>(result), static_cast<std::streamsize>(metadata.length));

    files.emplace_back(metadata, result);
}

void OMBundle::saveBundle(std::ostream &stream)
{
    // header
    stream.write(reinterpret_cast<const char *>(header.data()), header.size());
    auto i = 0;
    stream.write(reinterpret_cast<const char *>(&i), 2);

    auto ll = binary::be64ToNative(files.size());
    stream.write(reinterpret_cast<const char *>(&ll), sizeof(ll));

    auto currentOff = binary::be64ToNative(8 + sizeof(ll) + files.size() * 2 * sizeof(uint64_t));

#define OFFSWITCH currentOff = binary::be64ToNative(currentOff);

    for (auto itt : files)
    {
        stream.write(reinterpret_cast<const char *>(&currentOff), sizeof(currentOff));
        OFFSWITCH
        currentOff += 2 * sizeof(uint64_t) + itt.first.name.size() + 1 + itt.first.owner.size() + 1;
        OFFSWITCH

        stream.write(reinterpret_cast<const char *>(&currentOff), sizeof(currentOff));
        OFFSWITCH
        currentOff += itt.first.length;
        OFFSWITCH
    }

    for (auto itt : files)
    {
        auto tmp = binary::be64ToNative(itt.first.timestamp);
        stream.write(reinterpret_cast<const char *>(&tmp), sizeof(tmp));

        tmp = binary::be64ToNative(itt.first.length);
        stream.write(reinterpret_cast<const char *>(&tmp), sizeof(tmp));

        stream.write(itt.first.name.data(), itt.first.name.length());
        stream.write(reinterpret_cast<const char *>(&i), 1);
        stream.write(itt.first.owner.data(), itt.first.owner.length());
        stream.write(reinterpret_cast<const char *>(&i), 1);

        stream.write(reinterpret_cast<const char *>(itt.second), static_cast<std::streamsize>(itt.first.length));
    }
}

auto OMBundle::fetchMetadata(std::shared_ptr<std::istream> stream) -> OMBundleFileMetadata
{
    return {rdbe64(stream), rdbe64(stream), rdstring(stream), rdstring(stream)};
}

} // namespace openminecraft::specs::vfsbundle
