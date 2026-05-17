#include "openminecraft/vm/encoding/om_encoding_utf.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"

#include <cstdint>
#include <cstring>

namespace openminecraft::vm::encoding
{
std::tuple<elysia::jchar *, elysia::jsize> utf8ToUtf16New(std::string str)
{
    std::vector<elysia::jchar> data;
    for (auto itt = str.begin(); itt < str.end(); ++itt)
    {
        if ((*itt & 0b10000000) == 0)
        {
            data.push_back(static_cast<elysia::jchar>(*itt));
        }
        else if ((*itt >> 5) == 0b110)
        {
            elysia::jchar a = (*itt & 0b00011111) << 6;
            ++itt;
            a += (static_cast<uint8_t>(*itt) & 0b00111111);
            data.push_back(a);
        }
        else if ((*itt >> 4) == 0b1110)
        {
            elysia::jchar a = (*itt & 0b00001111) << 12;
            ++itt;
            a += (static_cast<uint8_t>(*itt) & 0b00111111) << 6;
            ++itt;
            a += (static_cast<uint8_t>(*itt) & 0b00111111);
            data.push_back(a);
        }
        else if ((*itt >> 3) == 0b11110)
        {
            int a = (*itt & 0b00000111) << 18;
            ++itt;
            a += (static_cast<uint8_t>(*itt) & 0b00111111) << 12;
            ++itt;
            a += (static_cast<uint8_t>(*itt) & 0b00111111) << 6;
            ++itt;
            a += (static_cast<uint8_t>(*itt) & 0b00111111);

            data.push_back(static_cast<elysia::jchar>(0b11011000 | ((a >> 18) & 0b11) | (a >> 10) & 0b11111111));
            data.push_back(static_cast<elysia::jchar>(0b11011100 | ((a >> 8) & 0b11) | a & 0b11111111));
        }
    }
    elysia::jchar *datar =
        reinterpret_cast<elysia::jchar *>(mem::allocator::tracedMallocElysia(sizeof(elysia::jchar) * data.size()));
    std::memcpy(datar, data.data(), data.size() * sizeof(elysia::jchar));
    return std::make_tuple(datar, data.size());
}
std::string utf16ToUtf8New(elysia::jchar *arr, elysia::jsize length)
{
    return "";
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
} // namespace openminecraft::vm::encoding
