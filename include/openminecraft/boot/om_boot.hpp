#ifndef OM_BOOT_HPP
#define OM_BOOT_HPP

#include "openminecraft/vm/pixeltower/v1/om_pixeltower_tracing.hpp"
#include <string>
#include <vector>

namespace openminecraft::boot
{
void onCrash(int code, int pid, std::vector<openminecraft::vm::pixeltower::v1::tracing::OMTracingFrame> &frames);
int boot(std::vector<std::string> args);

void pixeltowerDynTest();
void pixeltowerLoadTest();

void vulkanRendererTest();
void openglRendererTest();
} // namespace openminecraft::boot

#endif
