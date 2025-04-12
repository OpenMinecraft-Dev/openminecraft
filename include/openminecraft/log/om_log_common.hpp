#ifndef OM_LOG_COMMON_HPP
#define OM_LOG_COMMON_HPP

#include <cstdint>
#include <fmt/format.h>
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
            OMLogger(std::string name, std::ostream& stream, bool format);
            OMLogger(std::string name, void* obj);
            OMLogger(std::string name);
            ~OMLogger();
            void debug(std::string msg);
            template<typename... T>
            void debug(std::string msg, T... args)
            {
                debug(fmt::format(msg, args...));
            }
            void info(std::string msg);
            template<typename... T>
            void info(std::string msg, T... args)
            {
                info(fmt::format(msg, args...));
            }
            void warn(std::string msg);
            template<typename... T>
            void warn(std::string msg, T... args)
            {
                warn(fmt::format(msg, args...));
            }
            void error(std::string msg);
            template<typename... T>
            void error(std::string msg, T... args)
            {
                error(fmt::format(msg, args...));
            }
            void fatal(std::string msg);
            template<typename... T>
            void fatal(std::string msg, T... args)
            {
                fatal(fmt::format(msg, args...));
            }

        protected:
            std::string loggerName;
            std::ostream* target;
            bool enableFormat;

        private:
            void log(OMLogType type, std::string msg);
    };
}

#endif
