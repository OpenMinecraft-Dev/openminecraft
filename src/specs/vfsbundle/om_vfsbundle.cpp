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

        stream->seekg(off);
        auto fmd = fetchMetadata(stream);
        STRERR

        stream->seekg(fileoff);
        std::vector<uint8_t> data;
        for (uint64_t idx = 0; idx < fmd.length; idx++)
        {
            char c;
            stream->read(&c, 1);
            data.push_back(c);
            STRERR
        }

        files.push_back(std::make_pair(fmd, data));

        logger.info("{}", fmd);
        stream->seekg(pp);
    }
}

OMBundleFileMetadata OMBundle::fetchMetadata(std::shared_ptr<std::istream> stream)
{
    return {rdbe64(stream), rdbe64(stream), rdstring(stream), rdstring(stream)};
}

} // namespace openminecraft::specs::vfsbundle