#ifndef OM_UTIL_VERSION_HPP
#define OM_UTIL_VERSION_HPP

#include <cstdint>
#include <string>

namespace openminecraft::util
{
class OMVersion
{
  public:
    OMVersion(uint16_t major, uint16_t minor, uint16_t patch1, uint16_t patch2);
    OMVersion(uint16_t major, uint16_t minor, uint16_t patch1);
    OMVersion(uint32_t vkver);
    OMVersion() : majorVersion(0), minorVersion(0), patch1(0), patch2(0)
    {
    }
    ~OMVersion();

    auto toString() -> std::string;

    auto toVKVersion() -> int;
    auto toVKApiVersion() -> int;

    const uint16_t majorVersion;
    const uint16_t minorVersion;
    const uint16_t patch1;
    const uint16_t patch2;
};
} // namespace openminecraft::util

#endif
