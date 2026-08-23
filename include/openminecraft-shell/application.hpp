#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "openminecraft/log/om_log_common.hpp"
#include <vector>
namespace openminecraftshell
{
class OMApplication
{
  public:
    OMApplication(std::vector<std::string> args);
    ~OMApplication();

    auto entry() -> int;

  private:
    std::vector<std::string> args;
    openminecraft::log::OMLogger logger;
};
}; // namespace openminecraftshell

#endif