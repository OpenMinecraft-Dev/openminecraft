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
} // namespace openminecraft::log