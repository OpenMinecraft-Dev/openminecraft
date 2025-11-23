#include "openminecraft/specs/vfsbundle/om_vfsbundle.hpp"

#include <memory>

namespace openminecraft::specs::vfsbundle
{
bool operator!=(const std::array<char, 5> &hd, const std::array<uint8_t, 5> &rhs)
{
    for (int i = 0; i < 5; i++)
    {
        if (hd[i] != rhs[i])
        {
            return true;
        }
    }

    return false;
}

constexpr auto rdbe64 = [](std::shared_ptr<std::istream> stream) {
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
constexpr auto rdstring = [](std::shared_ptr<std::istream> stream) {
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

    return std::string(chs.begin(), chs.end());
};

OMBundle::OMBundle() : logger("OMBundle", this)
{
}

OMBundle::OMBundle(std::shared_ptr<std::istream> stream) : OMBundle()
{
    std::array<char, 5> hd = {};
    stream->read(hd.data(), 5);

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

        logger.info("{}", fmd);
        stream->seekg(pp);
    }

    isOnHeap = true;
}

OMBundle::~OMBundle()
{
    if (isOnHeap)
    {
        for (auto itt : files)
        {
            delete[] itt.second;
        }
    }
}


OMBundleFileMetadata OMBundle::fetchMetadata(std::shared_ptr<std::istream> stream)
{
    return {rdbe64(stream), rdbe64(stream), rdstring(stream), rdstring(stream)};
}

} // namespace openminecraft::specs::vfsbundle