#ifndef OM_BOOT_HPP
#define OM_BOOT_HPP

#include "openminecraft/renderer/om_renderer_window.hpp"
#include <string>
#include <vector>

using namespace openminecraft;

namespace openminecraftshell
{
// INFO: the boot function for the app
auto boot(std::vector<std::string> args) -> int;
void rendererLoop(renderer::OMBackend);
} // namespace openminecraftshell

#endif
