#ifndef OM_UTIL_VERSION_HPP
#define OM_UTIL_VERSION_HPP

#include <cstdint>
#include <string>

namespace openminecraft::util
{
class Version
{
  public:
    Version(uint16_t major, uint16_t minor, uint16_t patch1, uint16_t patch2);
    Version(uint16_t major, uint16_t minor, uint16_t patch1);
    ~Version();

    std::string toString();

    int toVKVersion();
    int toVKApiVersion();

    const uint16_t major;
    const uint16_t minor;
    const uint16_t patch1;
    const uint16_t patch2;
};
} // namespace openminecraft::util

#endif