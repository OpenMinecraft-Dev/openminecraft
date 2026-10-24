#include "openminecraft/util/om_util_version.hpp"
#include "fmt/format.h"
#include "vulkan/vulkan_core.h"

namespace openminecraft::util
{
OMVersion::OMVersion(uint16_t major, uint16_t minor, uint16_t patch1, uint16_t patch2)
    : majorVersion(major), minorVersion(minor), patch1(patch1), patch2(patch2)
{
}
OMVersion::OMVersion(uint16_t major, uint16_t minor, uint16_t patch1)
    : majorVersion(major), minorVersion(minor), patch1(patch1), patch2(0)
{
}
OMVersion::OMVersion(uint32_t vkver)
    : majorVersion(VK_VERSION_MAJOR(vkver)), minorVersion(VK_VERSION_MINOR(vkver)), patch1(VK_VERSION_PATCH(vkver)),
      patch2(0)
{
}
OMVersion::~OMVersion() = default;
auto OMVersion::toString() -> std::string
{
    return fmt::format("{}.{}.{}.{}", majorVersion, minorVersion, patch1, patch2);
}
auto OMVersion::toVKVersion() -> int
{
    return VK_MAKE_VERSION(majorVersion, minorVersion, patch1);
}
auto OMVersion::toVKApiVersion() -> int
{
    return VK_MAKE_API_VERSION(majorVersion, minorVersion, patch1, patch2);
}
} // namespace openminecraft::util
