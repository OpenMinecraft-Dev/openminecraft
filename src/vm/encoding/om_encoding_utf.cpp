#include "openminecraft/vm/encoding/om_encoding_utf.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <tuple>

namespace openminecraft::vm::encoding
{
std::tuple<elysia::jchar *, elysia::jsize> utf8ToUtf16New(std::string str)
{
    std::vector<elysia::jchar> data;
    data.reserve(1);
    const auto end = str.end();

    for (auto it = str.begin(); it != end; ++it)
    {
        uint8_t byte = static_cast<uint8_t>(*it);

        if ((byte & 0x80) == 0)
        {
            data.push_back(byte);
            continue;
        }

        int extra = 0;
        uint32_t codepoint = 0;
        uint8_t mask = 0;

        if ((byte >> 5) == 0b110)
        {
            extra = 1;
            mask = 0x1F;
            codepoint = byte & mask;
        }
        else if ((byte >> 4) == 0b1110)
        {
            extra = 2;
            mask = 0x0F;
            codepoint = byte & mask;
        }
        else if ((byte >> 3) == 0b11110)
        {
            extra = 3;
            mask = 0x07;
            codepoint = byte & mask;
        }
        else
        {
            continue;
        }

        if (std::distance(it, end) <= extra)
            break;

        bool valid = true;
        for (int i = 0; i < extra; ++i)
        {
            ++it;
            uint8_t next = static_cast<uint8_t>(*it);
            if ((next & 0xC0) != 0x80)
            {
                valid = false;
                break;
            }
            codepoint = (codepoint << 6) | (next & 0x3F);
        }
        if (!valid)
            continue;

        if (extra == 1 && codepoint < 0x80)
            continue;
        if (extra == 2 && codepoint < 0x800)
            continue;
        if (extra == 3 && codepoint < 0x10000)
            continue;
        if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
            continue;
        if (codepoint > 0x10FFFF)
            continue;

        if (codepoint <= 0xFFFF)
        {
            data.push_back(static_cast<elysia::jchar>(codepoint));
        }
        else
        {
            uint32_t u = codepoint - 0x10000;
            data.push_back(static_cast<elysia::jchar>(0xD800 | (u >> 10)));
            data.push_back(static_cast<elysia::jchar>(0xDC00 | (u & 0x3FF)));
        }
    }

    elysia::jchar *datar =
        reinterpret_cast<elysia::jchar *>(mem::allocator::tracedMallocElysia(sizeof(elysia::jchar) * data.size()));
    std::memcpy(datar, data.data(), data.size() * sizeof(elysia::jchar));
    return std::make_tuple(datar, data.size());
}
std::string utf16ToUtf8New(elysia::jchar *arr, elysia::jsize length)
{
    std::string result;
    for (elysia::jsize i = 0; i < length; ++i)
    {
        uint32_t codepoint = arr[i];

        if ((arr[i] & 0xFC00) == 0xD800 && i + 1 < length)
        {
            uint16_t low = arr[i + 1];
            if ((low & 0xFC00) == 0xDC00)
            {
                codepoint = 0x10000 + (((arr[i] & 0x3FF) << 10) | (low & 0x3FF));
                ++i;
            }
            else
            {
                continue;
            }
        }
        else if ((arr[i] & 0xFC00) == 0xDC00)
        {
            continue;
        }

        if (codepoint <= 0x7F)
        {
            result += static_cast<char>(codepoint);
        }
        else if (codepoint <= 0x7FF)
        {
            result += static_cast<char>(0xC0 | (codepoint >> 6));
            result += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
        else if (codepoint <= 0xFFFF)
        {
            result += static_cast<char>(0xE0 | (codepoint >> 12));
            result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
        else if (codepoint <= 0x10FFFF)
        {
            result += static_cast<char>(0xF0 | (codepoint >> 18));
            result += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
        else
        {
            result += "\xef\xbf\xbd";
        }
    }
    return result;
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
} // namespace openminecraft::vm::encoding
