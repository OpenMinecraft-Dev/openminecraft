#ifndef OM_BOOT_HPP
#define OM_BOOT_HPP

#include "openminecraft/renderer/om_renderer_window.hpp"
#include <string>
#include <vector>

namespace openminecraft::boot
{
// INFO: the boot function for the app
auto boot(std::vector<std::string> args) -> int;
void rendererTest(renderer::OMBackend);
} // namespace openminecraft::boot

#endif
