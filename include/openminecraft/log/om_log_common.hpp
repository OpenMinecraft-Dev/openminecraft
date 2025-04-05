#ifndef OM_LOG_COMMON_HPP
#define OM_LOG_COMMON_HPP

#include <cstdint>
#include <ostream>
#include <string>
#include <sstream>

#define omLogCat(a, b) omLogCatI(a, b)
#define omLogCatI(a, b) omLogCatII(~, a ## b)
#define omLogCatII(p, res) res
#define omLogNameUnique(base) omLogCat(base, __LINE__)
#define omLog(caller, f) std::stringstream omLogNameUnique(temp);omLogNameUnique(temp) << f; caller(omLogNameUnique(temp).str())

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
            OMLogger(std::string name);
            ~OMLogger();
            void debug(std::string msg);
            void info(std::string msg);
            void warn(std::string msg);
            void error(std::string msg);
            void fatal(std::string msg);

        protected:
            std::string loggerName;
            std::ostream* target;
            bool enableFormat;

        private:
            void log(OMLogType type, std::string msg);
    };
}

#endif
