#include <openminecraft/log/om_log_ansi.hpp>
#include <string>

namespace openminecraft::log::ansi
{
std::string toAnsi(int code)
{
#ifdef OM_PLATFORM_DESKTOP
    std::string t = "\033[";
    t.append(std::to_string(code)).append("m");
    return t;
#else
    return "";
#endif
}
} // namespace openminecraft::log::ansi