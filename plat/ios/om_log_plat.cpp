#include <openminecraft/log/om_log_plat.hpp>
#include <iostream>

namespace openminecraft::log
{
    std::ostream& getPlatformLoggingStream()
    {
        return std::cout;
    }

    bool enableFormatting()
    {
        return true;
    }
}