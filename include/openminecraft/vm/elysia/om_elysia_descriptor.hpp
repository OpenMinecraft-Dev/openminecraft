#ifndef OM_ELYSIA_DESCRIPTOR_HPP
#define OM_ELYSIA_DESCRIPTOR_HPP

#include "openminecraft/binary/om_bin_hash.hpp"
#include <string>

namespace openminecraft::vm::elysia
{
static std::string buildArray(char *s)
{
    using namespace openminecraft::binary::hash;
    switch (binary::hash::hash_compile_time(s))
    {
    case "byte"_hash:
        return "[B";
    case "char"_hash:
        return "[C";
    case "short"_hash:
        return "[S";
    case "int"_hash:
        return "[I";
    case "long"_hash:
        return "[L";
    case "float"_hash:
        return "[F";
    case "double"_hash:
        return "[D";
    case "boolean"_hash:
        return "[Z";
    default: {
        if (s[0] == '[')
        {
            return "[" + std::string(s);
        }
        else
        {
            return "[L" + std::string(s) + ";";
        }
    }
    }
}

static uint64_t fieldLength(char *s, uint64_t ptrLen)
{
    switch (s[0])
    {
    case 'B':
    case 'Z':
        return 1;
    case 'C':
    case 'S':
        return 2;
    case 'F':
    case 'I':
        return 4;
    case 'L':
    case 'D':
        return 8;
    default:
        return ptrLen;
    }
}
} // namespace openminecraft::vm::elysia

#endif
