#ifndef OM_BIN_HASH_HPP
#define OM_BIN_HASH_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace openminecraft::binary::hash
{
typedef std::uint64_t hash_t;
constexpr hash_t prime = 0x100000001B3ull;
constexpr hash_t basis = 0xCBF29CE484222325ull;

constexpr hash_t hash_compile_time(const char *str, hash_t last_value = basis)
{
    if (*str)
    {
        hash_t result = last_value;
        const char *current = str;
        while (*current)
        {
            result = (*current ^ result) * prime;
            current++;
        }
        return result;
    }
    return last_value;
}

constexpr unsigned long long operator""_hash(const char *p, size_t)
{
    return hash_compile_time(p);
}
} // namespace openminecraft::binary::hash

#endif