#include "openminecraft/util/om_util_version.hpp"
#include "fmt/format.h"
#include "vulkan/vulkan_core.h"

namespace openminecraft::util
{
Version::Version(uint16_t major, uint16_t minor, uint16_t patch1, uint16_t patch2)
    : major(major), minor(minor), patch1(patch1), patch2(patch2)
{
}
Version::Version(uint16_t major, uint16_t minor, uint16_t patch1)
    : major(major), minor(minor), patch1(patch1), patch2(0)
{
}
Version::Version(uint32_t vkver)
    : major(VK_VERSION_MAJOR(vkver)), minor(VK_VERSION_MINOR(vkver)), patch1(VK_VERSION_PATCH(vkver)), patch2(0)
{
}
Version::~Version()
{
}
std::string Version::toString()
{
    return fmt::format("{}.{}.{}.{}", major, minor, patch1, patch2);
}
int Version::toVKVersion()
{
    return VK_MAKE_VERSION(major, minor, patch1);
}
int Version::toVKApiVersion()
{
    return VK_MAKE_API_VERSION(major, minor, patch1, patch2);
}
} // namespace openminecraft::util