#ifndef OM_ELYSIA_DESCRIPTOR_HPP
#define OM_ELYSIA_DESCRIPTOR_HPP

#include "openminecraft/binary/om_bin_hash.hpp"
#include <cstdint>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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
constexpr uint8_t argTypeArray = 0x9;
constexpr uint8_t argTypeVoid = 0xa;

struct OMElysiaSignaturePart
{
    uint8_t type;
    int layerCount = 0;
    std::shared_ptr<OMElysiaSignaturePart> subpart;
    std::string content;

    void print()
    {
        switch (type)
        {
        case argTypeByte:
            std::cout << "byte" << std::endl;
            break;
        case argTypeBoolean:
            std::cout << "boolean" << std::endl;
            break;
        case argTypeChar:
            std::cout << "char" << std::endl;
            break;
        case argTypeShort:
            std::cout << "short" << std::endl;
            break;
        case argTypeInt:
            std::cout << "int" << std::endl;
            break;
        case argTypeFloat:
            std::cout << "float" << std::endl;
            break;
        case argTypeLong:
            std::cout << "long" << std::endl;
            break;
        case argTypeDouble:
            std::cout << "double" << std::endl;
            break;
        case argTypeVoid:
            std::cout << "primitive type" << std::endl;
            break;
        case argTypeReference:
            std::cout << "ref of " << content << std::endl;
            break;
        case argTypeArray:
            std::cout << "array of depth " << layerCount << ", type ";
            subpart->print();
            break;
        }
    }
};

inline static void parseSignaturePart(std::string sig, OMElysiaSignaturePart *part)
{
    auto str = sig.c_str();
    parseSignaturePart(str, part);
}
inline static void parseSignaturePart(const char *&sig, OMElysiaSignaturePart *part)
{
    switch (*sig)
    {
    case 'B':
        part->type = argTypeByte;
        ++sig;
        break;
    case 'C':
        part->type = argTypeChar;
        ++sig;
        break;
    case 'S':
        part->type = argTypeShort;
        ++sig;
        break;
    case 'Z':
        part->type = argTypeBoolean;
        ++sig;
        break;
    case 'F':
        part->type = argTypeFloat;
        ++sig;
        break;
    case 'I':
        part->type = argTypeInt;
        ++sig;
        break;
    case 'D':
        part->type = argTypeDouble;
        ++sig;
        break;
    case 'J':
        part->type = argTypeLong;
        ++sig;
        break;
    case '[':
        part->type = argTypeArray;
        do
        {
            part->layerCount++;
            ++sig;
        } while (*sig == '[');
        part->subpart = std::make_shared<OMElysiaSignaturePart>();
        parseSignaturePart(sig, part->subpart.get());
        break;
    case 'L': {
        part->type = argTypeReference;
        std::string s = "";
        ++sig;
        while (*sig != ';')
        {
            s += *sig;
            ++sig;
        }
        ++sig;
        part->content = s;
        break;
    }
    }
}

inline static std::pair<std::vector<OMElysiaSignaturePart>, OMElysiaSignaturePart> parseSignature(const char *sig)
{
    bool insideArgs = false;

    OMElysiaSignaturePart retValue;
    std::vector<OMElysiaSignaturePart> argTypes;

    while (true)
    {
        OMElysiaSignaturePart part = {};
        OMElysiaSignaturePart &target = insideArgs ? part : retValue;

        switch (*sig)
        {
        case '(':
            insideArgs = true;
            ++sig;
            break;
        case ')':
            insideArgs = false;
            ++sig;
            break;
        case 'B':
        case 'C':
        case 'S':
        case 'Z':
        case 'F':
        case 'I':
        case 'D':
        case 'J':
        case '[':
        case 'L':
            parseSignaturePart(sig, &target);
            break;
        case '\0':
            goto retRes;
        default:
            throw std::logic_error("invalid signature!");
        }

        if (insideArgs)
        {
            argTypes.push_back(part);
        }
    }

retRes:
    return std::make_pair(argTypes, retValue);
}

inline static std::string signatureToRaw(OMElysiaSignaturePart &part)
{
    switch (part.type)
    {
    case argTypeBoolean:
        return "Z";
    case argTypeByte:
        return "B";
    case argTypeShort:
        return "S";
    case argTypeChar:
        return "C";
    case argTypeFloat:
        return "F";
    case argTypeInt:
        return "I";
    case argTypeDouble:
        return "D";
    case argTypeLong:
        return "J";
    case argTypeReference:
        return "L" + part.content + ";";
    case argTypeVoid:
        return "V";
    case argTypeArray: {
        std::string s = "";
        for (int i = 0; i < part.layerCount; i++)
        {
            s += "[";
        }
        s += signatureToRaw(*part.subpart.get());
        return s;
    }
    default:
        return "";
    }
}

inline static std::string signatureToType(OMElysiaSignaturePart &part)
{
    switch (part.type)
    {
    case argTypeBoolean:
        return "boolean";
    case argTypeByte:
        return "byte";
    case argTypeShort:
        return "short";
    case argTypeChar:
        return "char";
    case argTypeFloat:
        return "float";
    case argTypeInt:
        return "int";
    case argTypeDouble:
        return "double";
    case argTypeLong:
        return "long";
    case argTypeReference:
        return part.content;
    case argTypeVoid:
        return "void";
    case argTypeArray: {
        std::string s = "";
        for (int i = 0; i < part.layerCount; i++)
        {
            s += "[";
        }
        s += signatureToRaw(*part.subpart.get());
        return s;
    }
    default:
        return "";
    }
}

static std::string fieldDescToType(const char *name)
{
    OMElysiaSignaturePart part;
    parseSignaturePart(name, &part);

    return signatureToType(part);
}

static std::string buildArray(char *s)
{
    using namespace openminecraft::binary::hash;
    switch (hash_compile_time(s))
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
    switch (s[1])
    {
    case 'Z':
        return "boolean";
    case 'B':
        return "byte";
    case 'C':
        return "char";
    case 'S':
        return "short";
    case 'I':
        return "int";
    case 'F':
        return "float";
    case 'J':
        return "long";
    case 'D':
        return "double";
    case 'L':
        return s.substr(2, s.length() - 3);
    default:
        return s.substr(1);
    }
}

static uint64_t descriptorLength(char *s, uint64_t ptrLen)
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

inline static std::vector<std::string> argDescriptorParse(char *desc)
{
    std::vector<std::string> st;
    ++desc;
    while (*desc)
    {
        if (*desc == ')')
            break;

        st.push_back(fieldDescToType(desc));

        if (*desc == 'L')
        {
            while (*desc != ';')
            {
                ++desc;
            }
        }

        ++desc;
    }

    for (auto &l : st)
    {
        std::cout << l << std::endl;
    }

    return st;
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

static uint64_t argCount(char *s)
{
    uint8_t argTypes[255];
    int argCount;
    uint8_t returnType;
    argDescriptorParse(s, argTypes, argCount, &returnType);
    return argCount;
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
