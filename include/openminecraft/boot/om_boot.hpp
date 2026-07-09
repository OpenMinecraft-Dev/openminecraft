#ifndef OM_BOOT_HPP
#define OM_BOOT_HPP

#include <string>
#include <vector>

namespace openminecraft::boot
{
auto boot(std::vector<std::string> args) -> int;

void pixeltowerDynTest();
void pixeltowerLoadTest();

void vulkanRendererTest();
void openglRendererTest();
} // namespace openminecraft::boot

#endif
