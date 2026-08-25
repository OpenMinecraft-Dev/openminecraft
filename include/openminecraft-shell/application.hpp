#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "SDL3/SDL_events.h"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/om_renderer_window.hpp"
#include <vector>
namespace openminecraftshell
{
class OMApplication
{
  public:
    OMApplication(std::vector<std::string> args);
    ~OMApplication();

    auto entry() -> int;
    void mainLoop(openminecraft::renderer::OMBackend backend);

  private:
    std::vector<std::string> args;
    openminecraft::log::OMLogger logger;
    bool isRunning = true;
};
}; // namespace openminecraftshell

#endif