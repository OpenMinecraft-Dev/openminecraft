#include "openminecraft/log/om_log_common.hpp"
#include <iostream>
#include <openminecraft/log/om_log_plat.hpp>

namespace openminecraft::log
{
auto getPlatformLoggingStream() -> std::ostream &
{
    return std::cout;
}

auto enableFormatting() -> bool
{
    return true;
}

void logExternal(OMLogType l, std::string msg, std::string name, std::string thr)
{
}
} // namespace openminecraft::log
