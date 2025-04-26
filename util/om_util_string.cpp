#include "openminecraft/util/om_util_string.hpp"
#include "openminecraft/log/om_log_common.hpp"

namespace openminecraft::util::string
{
log::OMLogger logger("test");
int utf8Next(std::shared_ptr<std::istream> s)
{
    int target = 0;
    char def = s->get();
    if ((def & 0b10000000) == 0)
    {
        target = def;
    }
    else if ((def & 0b11100000) == 0b11000000)
    {
        target = (def & 0b00011111) << 6;
        def = s->get();
        target += def & 0b00111111;
    }
    else if ((def & 0b11110000) == 0b11100000)
    {
        target = (def & 0b00001111) << 12;
        def = s->get();
        target += (def & 0b00111111) << 6;
        def = s->get();
        target += def & 0b00111111;
    }
    else if ((def & 0b11111000) == 0b11110000)
    {
        target = (def & 0b00000111) << 18;
        def = s->get();
        target += (def & 0b00111111) << 12;
        def = s->get();
        target += (def & 0b00111111) << 6;
        def = s->get();
        target += def & 0b00111111;
    }
    return target;
}
} // namespace openminecraft::util::string