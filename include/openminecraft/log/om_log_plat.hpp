#ifndef OM_LOG_PLAT_HPP
#define OM_LOG_PLAT_HPP

#include <ostream>

namespace openminecraft::log {
    std::ostream& getPlatformLoggingStream();
}

#endif