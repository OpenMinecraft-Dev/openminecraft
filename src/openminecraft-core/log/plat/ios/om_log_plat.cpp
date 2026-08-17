#include "openminecraft/log/om_log_common.hpp"
#include <iostream>
#include <openminecraft/log/om_log_plat.hpp>
#include <os/log.h>

namespace openminecraft::log
{
auto getPlatformLoggingStream() -> std::ostream &
{
    return std::cout;
}

auto enableFormatting() -> bool
{
    return false;
}

void logExternal(OMLogType l, std::string msg, std::string name, std::string thr)
{
    os_log(OS_LOG_DEFAULT, "[OpenMinecraft Loggging] %s: %s", name.c_str(), msg.c_str());
}
} // namespace openminecraft::log
