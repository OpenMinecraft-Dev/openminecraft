#include "openminecraft/log/om_log_ansi.hpp"
#include "openminecraft/log/om_log_plat.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include <cstdarg>
#include <ctime>
#include <iomanip>
#include <openminecraft/log/om_log_common.hpp>
#include <string>
#include <thread>

using namespace openminecraft::log::ansi;

namespace openminecraft::log
{
    OMLogger::OMLogger(std::string name, std::ostream& stream, bool format)
    {
        this->loggerName = name;
        this->target = &stream;
        this->enableFormat = format;
    }

    OMLogger::OMLogger(std::string name): OMLogger(name, getPlatformLoggingStream(), enableFormatting()) {}

    OMLogger::~OMLogger()
    {
        delete this->target;
    }

    void OMLogger::debug(std::string msg) { log(Debug, msg); }
    void OMLogger::info(std::string msg) { log(Info, msg); }
    void OMLogger::warn(std::string msg) { log(Warn, msg); }
    void OMLogger::error(std::string msg) { log(Error, msg); }
    void OMLogger::fatal(std::string msg) { log(Fatal, msg); }

    void OMLogger::log(OMLogType type, std::string msg)
    {
        if (this->enableFormat)
        {
            auto now = time(nullptr);
            auto ltm = localtime(&now);

            *this->target << OMLogAnsiReset << OMLogAnsiBlueLight << "[" << std::setw(2) << std::setfill('0') << ltm->tm_hour << ":";
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
            *this->target << multithraad::acquireThreadName(std::this_thread::get_id()) << "] ";

            *this->target << OMLogAnsiCyan << "(" << this->loggerName << ") " << OMLogAnsiReset;
            *this->target << msg << std::endl;
        }
        else
        {
            *this->target << (char) type << msg;
        }
    }
}
