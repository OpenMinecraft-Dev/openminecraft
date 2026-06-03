#ifndef OM_ELYSIA_DESCRIPTOR_HPP
#define OM_ELYSIA_DESCRIPTOR_HPP

#include "openminecraft/binary/om_bin_hash.hpp"
#include <cstdint>
#include <string>

namespace openminecraft::vm::elysia
{
constexpr uint8_t argTypeByte = 0x0;
constexpr uint8_t argTypeBoolean = 0x1;
constexpr uint8_t argTypeChar = 0x2;
constexpr uint8_t argTypeShort = 0x3;
constexpr uint8_t argTypeInt = 0x4;
constexpr uint8_t argTypeFloat = 0x5;
constexpr uint8_t argTypeLong = 0x6;
constexpr uint8_t argTypeDouble = 0x7;
constexpr uint8_t argTypeReference = 0x8;
constexpr uint8_t argTypeVoid = 0x9;

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
        return "[J";
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

static bool isArray(std::string s)
{
    return s[0] == '[';
}

static std::string decompArray(std::string s)
{
    if (s[1] == 'L')
    {
        return s.substr(2, s.length() - 3);
    }
    else
    {
        return s.substr(1);
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
    case 'J':
    case 'D':
        return 8;
    default:
        return ptrLen;
    }
}

inline static int argToSlot(uint8_t *out, int argCount)
{
    int l = 0;
    for (int i = 0; i < argCount; i++)
    {
        if (out[i] == argTypeLong || out[i] == argTypeDouble)
        {
            l += 2;
        }
        else
        {
            ++l;
        }
    }

    return l;
}
inline static void argDescriptorParse(char *desc, uint8_t *out, int &argCount, uint8_t *returnType, int maxArgs = 255)
{
    argCount = 0;
    bool inArg = false;
#define checkArg                                                                                                       \
    if (inArg)                                                                                                         \
    {                                                                                                                  \
        ++argCount;                                                                                                    \
    }

#define precheck(type)                                                                                                 \
    if (!inArg)                                                                                                        \
    {                                                                                                                  \
        *returnType = type;                                                                                            \
    }

    while (*desc)
    {
        switch (*desc)
        {
        case '(':
            inArg = true;
            break;
        case 'Z':
            precheck(argTypeBoolean);
            out[argCount] = argTypeBoolean;
            checkArg;
            break;
        case 'B':
            precheck(argTypeByte);
            out[argCount] = argTypeByte;
            checkArg;
            break;
        case 'C':
            precheck(argTypeChar);
            out[argCount] = argTypeChar;
            checkArg;
            break;
        case 'S':
            precheck(argTypeShort);
            out[argCount] = argTypeShort;
            checkArg;
            break;
        case 'I':
            precheck(argTypeInt);
            out[argCount] = argTypeInt;
            checkArg;
            break;
        case 'F':
            precheck(argTypeFloat);
            out[argCount] = argTypeFloat;
            checkArg;
            break;
        case 'J':
            precheck(argTypeLong);
            out[argCount] = argTypeLong;
            checkArg;
            break;
        case 'D':
            precheck(argTypeDouble);
            out[argCount] = argTypeDouble;
            checkArg;
            break;
        case 'L':
            if (!inArg)
            {
                *returnType = argTypeReference;
                while (*desc != ';')
                {
                    ++desc;
                }
                break;
            }
            out[argCount] = argTypeReference;
            checkArg;
            while (*desc != ';')
            {
                ++desc;
            }
            break;
        case ')':
            inArg = false;
            break;
        case 'V':
            if (!inArg)
            {
                *returnType = argTypeVoid;
            }
            break;
        }
        ++desc;
    }
}

static uint64_t argSlots(char *s)
{
    uint8_t argTypes[255];
    int argCount;
    uint8_t returnType;
    argDescriptorParse(s, argTypes, argCount, &returnType);
    return argToSlot(argTypes, argCount);
}
} // namespace openminecraft::vm::elysia

#endif
