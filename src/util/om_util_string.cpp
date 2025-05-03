#include "openminecraft/util/om_util_string.hpp"
#include "openminecraft/log/om_log_common.hpp"

namespace openminecraft::util::string
{
log::OMLogger logger("test");
std::string uniToString(std::vector<int> arr)
{
    std::vector<char> data;
    for (auto i : arr)
    {
        if (i < 0b10000000)
        {
            data.push_back((char)i);
        }
        else if (i < 0b0000100000000000)
        {
            data.push_back((char)(i >> 6 | 0b11000000));
            data.push_back((char)((i & 0b00111111) | 0b10000000));
        }
        else if (i < 0b000000010000000000000000)
        {
            data.push_back((char)(i >> 12 | 0b11100000));
            data.push_back((char)((i >> 6 | 0b00111111) | 0b10000000));
            data.push_back((char)((i | 0b00111111) | 0b10000000));
        }
        else if (i < 0b001000000000000000000000)
        {
            data.push_back((char)(i >> 18 | 0b11110000));
            data.push_back((char)((i >> 12 | 0b00111111) | 0b10000000));
            data.push_back((char)((i >> 6 | 0b00111111) | 0b10000000));
            data.push_back((char)((i | 0b00111111) | 0b10000000));
        }
    }
    data.push_back('\0');
    return data.data();
}
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