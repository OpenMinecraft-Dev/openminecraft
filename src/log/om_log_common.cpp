#include "boost/stacktrace/detail/location_from_symbol.hpp"
#include "boost/stacktrace/frame.hpp"
#include "boost/stacktrace/stacktrace.hpp"
#include "openminecraft/log/om_log_ansi.hpp"
#include "openminecraft/log/om_log_plat.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include <ctime>
#include <fmt/format.h>
#include <iomanip>
#include <iostream>
#include <openminecraft/log/om_log_common.hpp>
#include <sstream>
#include <string>
#include <thread>

using namespace openminecraft::log::ansi;

namespace openminecraft::log
{
static std::string composeName(std::string name, void *obj)
{
    std::stringstream s;
    s << name << " @ 0x" << std::hex << obj;
    return s.str();
}

OMLogger::OMLogger(std::string name, std::ostream &stream, bool format)
{
    this->loggerName = name;
    this->target = &stream;
    this->enableFormat = format;
}

OMLogger::OMLogger(std::string name) : OMLogger(name, getPlatformLoggingStream(), enableFormatting())
{
}

OMLogger::OMLogger(std::string name, void *obj) : OMLogger(composeName(name, obj))
{
}

OMLogger::~OMLogger()
{
}

void OMLogger::debug(std::string msg)
{
    log(Debug, msg);
}
void OMLogger::info(std::string msg)
{
    log(Info, msg);
}
void OMLogger::warn(std::string msg)
{
    log(Warn, msg);
}
void OMLogger::error(std::string msg)
{
    log(Error, msg);
}
void OMLogger::fatal(std::string msg)
{
    log(Fatal, msg);
}

void OMLogger::dumpStacktrace()
{
    auto st = boost::stacktrace::stacktrace();
    std::string imageName;
    std::string filename;
    int fileline = 0;
    int id = st.size();
    for (auto frame : st)
    {
        id--;
        auto cur = boost::stacktrace::detail::location_from_symbol(frame.address()).name();
        if (imageName != cur)
        {
            fatal("-> {}", cur);
            imageName = cur;
        }
        if (filename != frame.source_file())
        {
            fileline = frame.source_line();
            fatal("*> {}:{}", frame.source_file(), fileline);
            filename = frame.source_file();
        }
        error("#{} {} @ {}", id, frame.address(), frame.name() == "" ? "???" : frame.name());
    }
}

void OMLogger::log(OMLogType type, std::string msg)
{
    if (this->enableFormat)
    {
        auto now = time(nullptr);
        auto ltm = localtime(&now);

        *this->target << OMLogAnsiReset << OMLogAnsiBlueLight << "[" << std::setw(2) << std::setfill('0')
                      << ltm->tm_hour << ":";
        *this->target << std::setw(2) << std::setfill('0') << ltm->tm_min << ":";
        *this->target << std::setw(2) << std::setfill('0') << ltm->tm_sec << "] ";

        switch (type)
        {
        case Debug:
            *this->target << OMLogAnsiBlue << "[debug/";
            break;
        case Info:
            *this->target << OMLogAnsiGreen << "[info/";
            break;
        case Warn:
            *this->target << OMLogAnsiYellowLight << "[warn/";
            break;
        case Error:
            *this->target << OMLogAnsiRedLight << "[error/";
            break;
        case Fatal:
            *this->target << OMLogAnsiRed << "[fatal/";
            break;
        default:
            *this->target << OMLogAnsiFaint << "[";
            break;
        }
        *this->target << multithread::acquireThreadName(std::this_thread::get_id()) << "] ";

        *this->target << OMLogAnsiCyan << "(" << this->loggerName << ") " << OMLogAnsiReset;
        *this->target << msg << std::endl;
    }
    else
    {
        logExternal(type, msg, loggerName, multithread::acquireThreadName(std::this_thread::get_id()));
    }
}
} // namespace openminecraft::log
