#include <openminecraft/log/om_log_plat.hpp>
#include <iostream>

std::ostream& getPlatformLoggingStream()
{
    return std::cout;
}