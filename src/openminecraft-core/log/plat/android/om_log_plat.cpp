#include "android/log.h"
#include "fmt/format.h"
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
    return false;
}
void logExternal(OMLogType l, std::string msg, std::string name, std::string thr)
{
    int prio = ANDROID_LOG_INFO;
    switch (l)
    {
    case Debug:
        prio = ANDROID_LOG_DEBUG;
        break;
    case Info:
        prio = ANDROID_LOG_INFO;
        break;
    case Warn:
        prio = ANDROID_LOG_WARN;
        break;
    case Error:
        prio = ANDROID_LOG_ERROR;
        break;
    case Fatal:
        prio = ANDROID_LOG_FATAL;
        break;
    default:
        break;
    }
    __android_log_print(prio, fmt::format("{}/{}", name, thr).c_str(), "%s", msg.c_str());
}
} // namespace openminecraft::log