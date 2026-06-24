#ifndef OM_VFSBUNDLE_HPP
#define OM_VFSBUNDLE_HPP

#include "openminecraft/log/om_log_common.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <fmt/chrono.h>
#include <iostream>
#include <memory>
#include <vector>

// geopelia: basic structure of the .vfsbundle file
// Header: 4f 4d 56 46 53 00 (OMVFS\0) + 00 00 (reserved)
// File map: (length: u64be) + length * ((file metadata offset: u64be) + (file data offset: u64be))
// Data: (pure binary data stream)
// Metadata structure: (timestamp: u64be) + (length: u64be) + (filename: null ended string) + (owner: null end string)

namespace openminecraft::specs::vfsbundle
{
struct OMBundleFileMetadata
{
    uint64_t timestamp;
    uint64_t length;
    std::string name;
    std::string owner;
};

constexpr std::array<uint8_t, 6> header = {0x4f, 0x4d, 0x56, 0x46, 0x53, 0x00};

class OMBundle
{
  public:
    OMBundle(std::shared_ptr<std::istream> stream);
    OMBundle(void *, uint64_t);
    OMBundle();
    ~OMBundle();

    void appendFile(OMBundleFileMetadata metadata, std::istream &stream);
    void saveBundle(std::ostream &stream);
    std::vector<std::pair<OMBundleFileMetadata, uint8_t *>> files;

  private:
    log::OMLogger logger;

    OMBundleFileMetadata fetchMetadata(std::shared_ptr<std::istream> stream);
    bool writable = false;
};
} // namespace openminecraft::specs::vfsbundle

template <> struct fmt::formatter<openminecraft::specs::vfsbundle::OMBundleFileMetadata> : formatter<string_view>
{
    auto format(const openminecraft::specs::vfsbundle::OMBundleFileMetadata &c, format_context &ctx) const
        -> format_context::iterator
    {
        return format_to(ctx.out(), "FileMetadata (timestamp={:%Y-%m-%d %H:%M}, size={}, path={}, owner={})",
                         std::chrono::system_clock::from_time_t(static_cast<time_t>(c.timestamp)), c.length, c.name,
                         c.owner);
    }
};

// FMT_FORMAT_AS()

#endif
