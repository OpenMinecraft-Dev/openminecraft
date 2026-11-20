#ifndef OM_BIN_HASH_HPP
#define OM_BIN_HASH_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

/*
 * INFO:
 * Simplee hash utility used in the project
 * Google CityHash algorithm
 */
namespace openminecraft::binary::hash
{
// INFO: special datatype for hash result
using hash_t = std::uint64_t;
// INFO: constants for hash
constexpr hash_t prime = 0x100000001B3ull;
constexpr hash_t basis = 0xCBF29CE484222325ull;

// INFO: compile time hash function inplementation
constexpr auto hash_compile_time(const char *str, hash_t last_value = basis) -> hash_t
{
    if (*str)
    {
        hash_t result = last_value;
        const char *current = str;
        while (*current)
        {
            // core calculation step: r = (c ^ r) * p
            // natural integer overflow
            result = (*current ^ result) * prime;
            current++;
        }
        return result;
    }
    return last_value;
}

// INFO: _hash operator for string constants
constexpr auto operator""_hash(const char *p, size_t) -> unsigned long long
{
    return hash_compile_time(p);
}
} // namespace openminecraft::binary::hash

#endif
