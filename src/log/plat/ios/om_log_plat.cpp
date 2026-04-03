#include "openminecraft/log/om_log_common.hpp"
#include <iostream>
#include <openminecraft/log/om_log_plat.hpp>

namespace openminecraft::log
{
std::ostream &getPlatformLoggingStream()
{
    return std::cout;
}

bool enableFormatting()
{
    return true;
}
void logExternal(OMLogType l, std::string msg, std::string name, std::string thr)
{
}
} // namespace openminecraft::log