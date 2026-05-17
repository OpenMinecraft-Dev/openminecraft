#include "openminecraft/vm/encoding/om_encoding_utf.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"

#include <cstdint>

namespace openminecraft::vm::encoding
{
elysia::jchar *utf8ToUtf16New(std::string str)
{
    std::vector<elysia::jchar> data;
    for (auto itt = str.begin(); itt < str.end(); ++itt)
    {
    }
    return nullptr;
}
std::string utf16ToUtf8New(elysia::jchar *arr, elysia::jsize length);

std::vector<int> utf8ToUtf32(std::string n)
{
    std::vector<int> target;
    for (auto itt = n.begin(); itt != n.end(); ++itt)
    {
        uint8_t cp = *itt;

        if ((cp & 0b10000000) == 0)
        {
            target.push_back(*itt);
        }
        else if ((cp >> 5) == 0b110)
        {
            int a = (cp & 0b00011111) << 6;
            ++itt;
            a += (static_cast<uint8_t>(*itt) & 0b00111111);
            target.push_back(a);
        }
        else if ((cp >> 4) == 0b1110)
        {
            int a = (cp & 0b00001111) << 12;
            ++itt;
            a += (static_cast<uint8_t>(*itt) & 0b00111111) << 6;
            ++itt;
            a += (static_cast<uint8_t>(*itt) & 0b00111111);
            target.push_back(a);
        }
        else if ((cp >> 3) == 0b11110)
        {
            int a = (cp & 0b00000111) << 18;
            ++itt;
            a += (static_cast<uint8_t>(*itt) & 0b00111111) << 12;
            ++itt;
            a += (static_cast<uint8_t>(*itt) & 0b00111111) << 6;
            ++itt;
            a += (static_cast<uint8_t>(*itt) & 0b00111111);
            target.push_back(a);
        }
    }

    return target;
}

std::string utf32ToUtf8(std::vector<int> cps)
{
    std::vector<uint8_t> s;
    for (auto i : cps)
    {
        if (i <= 0x7f)
        {
            s.push_back(static_cast<uint8_t>(i));
        }
        else if (i <= 0x7ff)
        {
            s.push_back(static_cast<uint8_t>(0b11000000 | (i >> 6)));
            s.push_back(static_cast<uint8_t>(0b10000000 | (i & 0b00111111)));
        }
        else if (i <= 0xffff)
        {
            s.push_back(static_cast<uint8_t>(0b11100000 | (i >> 12)));
            s.push_back(static_cast<uint8_t>(0b10000000 | ((i >> 6) & 0b00111111)));
            s.push_back(static_cast<uint8_t>(0b10000000 | (i & 0b00111111)));
        }
        else if (i <= 0x10ffff)
        {
            s.push_back(static_cast<uint8_t>(0b11110000 | (i >> 18)));
            s.push_back(static_cast<uint8_t>(0b10000000 | ((i >> 12) & 0b00111111)));
            s.push_back(static_cast<uint8_t>(0b10000000 | ((i >> 6) & 0b00111111)));
            s.push_back(static_cast<uint8_t>(0b10000000 | (i & 0b00111111)));
        }
    }
    auto ss = std::string(s.begin(), s.end());
    return ss;
}
std::vector<int> utf16ToUtf32(std::vector<uint8_t> d)
{
    std::vector<int> res;
    for (auto itt = d.begin(); itt != d.end(); ++itt)
    {
        if ((*itt >> 2) == 0b110110)
        {
            ++itt;
            ++itt;
            if ((*itt >> 2) == 0b110111)
            {
                --itt;
                --itt;

                int i = (*itt & 0b11) << 18;
                ++itt;
                i |= *itt << 10;
                ++itt;
                i |= (*itt & 0b11) << 8;
                ++itt;
                i |= *itt;
                res.push_back(i);
            }
            else
            {
                --itt;
                --itt;
                goto normal;
            }
        }
        else
        {
        normal:
            int i = *itt << 8;
            ++itt;
            i |= *itt;
            res.push_back(i);
        }
    }
    return res;
}
std::vector<uint8_t> utf32ToUtf16(std::vector<int> cps)
{
    std::vector<uint8_t> s;
    for (auto i : cps)
    {
        if (i <= 0xffff)
        {
            s.push_back(i >> 8);
            s.push_back(i);
        }
        else
        {
            s.push_back(0b11011000 | ((i >> 18) & 0b11));
            s.push_back((i >> 10) & 0b11111111);
            s.push_back(0b11011100 | ((i >> 8) & 0b11));
            s.push_back(i & 0b11111111);
        }
    }
    return s;
}
} // namespace openminecraft::vm::encoding
