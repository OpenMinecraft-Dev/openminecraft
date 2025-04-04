#ifndef OM_LOG_COMMON_HPP
#define OM_LOG_COMMON_HPP

#include <cstdint>
#include <ostream>
#include <string>

namespace openminecraft::log
{
    enum OMLogType: uint8_t
    {
        Debug, 
        Info,
        Warn,
        Error,
        Fatal
    };

    class OMLogger
    {
        public:
            OMLogger(std::string name, std::ostream& stream);
            ~OMLogger();
            void debug(std::string msg);
            void info(std::string msg);
            void warn(std::string msg);
            void error(std::string msg);
            void fatal(std::string msg);

        protected:
            std::string loggerName;
            std::ostream* target;

        private:
            void log(OMLogType type, std::string msg);
    };
}

#endif