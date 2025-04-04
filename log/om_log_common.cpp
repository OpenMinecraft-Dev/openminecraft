#include <openminecraft/log/om_log_common.hpp>

namespace openminecraft::log
{
    OMLogger::OMLogger(std::string name, std::ostream& stream)
    {
        this->loggerName = name;
        this->target = &stream;
    }

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
        *this->target << msg.append("\n");
    }
}