#ifndef OM_ELYSIA_DESCRIPTOR_HPP
#define OM_ELYSIA_DESCRIPTOR_HPP

#include "openminecraft/binary/om_bin_hash.hpp"
#include <cstdint>
#include <memory>
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
};

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
    case 'V':
        part->type = argTypeVoid;
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
        part->content = "";
        const char *start = ++sig;
        while (*sig != ';')
        {
            ++sig;
        }
        part->content.assign(start, sig);
        ++sig;
        break;
    }
    }
}

inline static void parseSignaturePart(std::string sig, OMElysiaSignaturePart *part)
{
    auto str = sig.c_str();
    parseSignaturePart(str, part);
}

inline static auto parseSignature(const char *sig)
    -> std::pair<std::vector<OMElysiaSignaturePart>, OMElysiaSignaturePart>
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
            continue;
        case ')':
            insideArgs = false;
            ++sig;
            continue;
        case 'B':
        case 'C':
        case 'S':
        case 'Z':
        case 'F':
        case 'I':
        case 'D':
        case 'J':
        case 'V':
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
            argTypes.emplace_back(part);
        }
    }

retRes:
    return {argTypes, retValue};
}

inline static auto signatureToRaw(OMElysiaSignaturePart &part) -> std::string
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

inline static auto signatureToType(OMElysiaSignaturePart &part) -> std::string
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

static inline auto fieldDescToType(const char *name) -> std::string
{
    OMElysiaSignaturePart part;
    parseSignaturePart(name, &part);

    return signatureToType(part);
}

static inline auto buildArray(char *s) -> std::string
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

static inline auto isArray(std::string s) -> bool
{
    return s[0] == '[';
}

static inline auto decompArray(std::string s) -> std::string
{
    OMElysiaSignaturePart part;
    parseSignaturePart(s, &part);

    if (part.layerCount <= 1)
    {
        return signatureToType(*part.subpart.get());
    }
    else
    {
        part.layerCount--;
        return signatureToType(part);
    }
}

static inline auto descriptorLength(char *s, uint64_t ptrLen) -> uint64_t
{
    switch (s[0])
    {
    case 'V':
        return 0;
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

inline static void descriptorTypes(
    std::shared_ptr<std::pair<std::vector<OMElysiaSignaturePart>, OMElysiaSignaturePart>> d, uint8_t *out,
    int &argCount, uint8_t *returnType, int maxArgs = 255)
{
    *returnType = d->second.type;

    for (int i = 0; i < d->first.size(); i++)
    {
        out[i] = d->first[i].type;
    }
    argCount = d->first.size();
}

static inline auto argCount(char *s) -> uint64_t
{
    return parseSignature(s).first.size();
}

static inline auto argSlots(char *s) -> uint64_t
{
    auto result = parseSignature(s);
    int argCount = 0;
    for (auto &p : result.first)
    {
        if (p.type == argTypeLong || p.type == argTypeDouble)
        {
            argCount += 2;
        }
        else
        {
            ++argCount;
        }
    }
    return argCount;
}
} // namespace openminecraft::vm::elysia

#endif
