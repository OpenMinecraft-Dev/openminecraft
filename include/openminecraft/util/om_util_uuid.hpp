#ifndef OM_UTIL_UUID_HPP
#define OM_UTIL_UUID_HPP

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <stdexcept>

namespace openminecraft::util
{
class OMUuid
{
  public:
    OMUuid() : OMUuid(0, 0)
    {
    }
    OMUuid(uint64_t low, uint64_t high) : low(low), high(high)
    {
    }

    OMUuid(std::string s)
    {
        std::string hex;
        for (char c : s)
        {
            if (std::isxdigit(static_cast<unsigned char>(c)))
            {
                hex.push_back(c);
            }
        }

        if (hex.size() != 32)
        {
            throw std::invalid_argument("Invalid UUID string: expected 32 hex digits");
        }

        auto parseHex64 = [](const std::string &hexStr, size_t start) -> uint64_t {
            uint64_t val = 0;
            for (size_t i = 0; i < 16; ++i)
            {
                char c = hexStr[start + i];
                val <<= 4;
                if (c >= '0' && c <= '9')
                    val |= (c - '0');
                else if (c >= 'a' && c <= 'f')
                    val |= (c - 'a' + 10);
                else if (c >= 'A' && c <= 'F')
                    val |= (c - 'A' + 10);
                else
                    throw std::invalid_argument("Invalid hex character");
            }
            return val;
        };

        high = parseHex64(hex, 0);
        low = parseHex64(hex, 16);
    }

    auto toString() const -> std::string
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        ss << std::setw(16) << high;
        ss << std::setw(16) << low;

        std::string hex = ss.str();

        return hex.substr(0, 8) + "-" + hex.substr(8, 4) + "-" + hex.substr(12, 4) + "-" + hex.substr(16, 4) + "-" +
               hex.substr(20, 12);
    }

    auto lowBits() const -> uint64_t
    {
        return low;
    }
    auto highBits() const -> uint64_t
    {
        return high;
    }

  private:
    uint64_t low, high;
};
} // namespace openminecraft::util

#endif