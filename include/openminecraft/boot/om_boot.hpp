#ifndef OM_BOOT_HPP
#define OM_BOOT_HPP

#include <string>
#include <vector>

namespace openminecraft::boot
{
int boot(std::vector<std::string> args);

void pixeltowerDynTest();
void pixeltowerLoadTest();

void vulkanRendererTest();
void openglRendererTest();
} // namespace openminecraft::boot

#endif
