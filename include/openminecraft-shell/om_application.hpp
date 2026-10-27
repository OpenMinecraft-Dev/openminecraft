#ifndef OM_APPLICATION_HPP
#define OM_APPLICATION_HPP

#include "SDL3/SDL_events.h"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/om_renderer_window.hpp"
#include <vector>
namespace openminecraft::shell
{
constexpr int EventMainScreenOpen = 0;
constexpr int EventMainScreenClose = 1;
constexpr int EventQuit = 2;
constexpr int EventMainScreenOpenFinish = 3;
constexpr int EventMainScreenCloseFinish = 4;

class OMApplication
{
  public:
    OMApplication(std::vector<std::string> args);
    ~OMApplication();

    auto entry() -> int;
    void mainLoop(openminecraft::renderer::OMBackend backend);
    void networkSetup(std::string, std::string);

  private:
    std::vector<std::string> args;
    openminecraft::log::OMLogger logger;
    bool isRunning = true;
};
}; // namespace openminecraft::shell

#endif