#ifndef OM_LOG_PLAT_HPP
#define OM_LOG_PLAT_HPP

#include <ostream>

namespace openminecraft::log
{
auto getPlatformLoggingStream() -> std::ostream &;
auto enableFormatting() -> bool;
} // namespace openminecraft::log

#endif
