#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "fmt/format.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include <typeindex>
#include <variant>
#include <vector>

using namespace openminecraft::binary::hash;

namespace openminecraft::vm::bytecode::descriptor
{
OMResult<std::string, std::string> decodeType(std::string raw, int *p)
{
    if (*p >= raw.length())
    {
        return OMResult<std::string, std::string>::err("string end");
    }
    *p = *p + 1;
    switch (raw[*p - 1])
    {
    case 'V':
        return OMResult<std::string, std::string>::ok("void");
    case 'B':
        return OMResult<std::string, std::string>::ok("byte");
    case 'C':
        return OMResult<std::string, std::string>::ok("char");
    case 'D':
        return OMResult<std::string, std::string>::ok("double");
    case 'F':
        return OMResult<std::string, std::string>::ok("float");
    case 'I':
        return OMResult<std::string, std::string>::ok("int");
    case 'J':
        return OMResult<std::string, std::string>::ok("long");
    case 'S':
        return OMResult<std::string, std::string>::ok("short");
    case 'Z':
        return OMResult<std::string, std::string>::ok("boolean");
    case 'L': {
        auto ends = raw.find_first_of(';', *p - 1);
        if (ends == std::variant_npos)
        {
            return OMResult<std::string, std::string>::err("nonstop object type!");
        }
        // inplace split
        raw[ends] = '\0';
        auto type = std::string(raw.substr(*p, ends).c_str());
        auto d = OMResult<std::string, std::string>::ok(type == "java/lang/String" ? "string" : type);
        raw[ends] = ';';
        *p = ends + 1;
        return d;
    }
    case '[': {
        auto sup = decodeType(raw, p);
        switch (sup.type)
        {
        case Ok:
            return OMResult<std::string, std::string>::ok(fmt::format("[{}", sup.unwrap()));
        case Err:
            return OMResult<std::string, std::string>::err(sup.unwrap_err());
        }
    }
    }
    *p = *p - 1;
    return OMResult<std::string, std::string>::err("not recognized");
}
OMResult<methodSig, std::string> decodeSignature(std::string raw, int *p)
{
    if (raw[*p] != '(')
    {
        return OMResult<methodSig, std::string>::err("not a method signature");
    }
    *p = *p + 1;
    std::vector<std::string> args;
    while (raw[*p] != ')')
    {
        auto data = decodeType(raw, p);
        switch (data.type)
        {
        case Ok:
            args.push_back(data.unwrap());
            break;
        case Err:
            return OMResult<methodSig, std::string>::err(data.unwrap_err());
        }
    }
    *p = *p + 1;

    auto ret = decodeType(raw, p);
    switch (ret.type)
    {
    case Ok:
        return OMResult<methodSig, std::string>::ok(methodSig(args, ret.unwrap()));
    case Err:
        return OMResult<methodSig, std::string>::err(ret.unwrap_err());
    }
}
std::any createEmptyData(std::string type)
{
    switch (hash_compile_time(type.c_str()))
    {
    case "byte"_hash:
    case "char"_hash:
    case "short"_hash:
    case "int"_hash:
    case "boolean"_hash:
        return 0;
    case "long"_hash:
        return (int64_t)0;
    case "float"_hash:
        return 0.f;
    case "double"_hash:
        return (double)0;
    case "java/lang/String"_hash:
        return "";
    default:
        return TempNonPrimitiveVariable{type};
    }
}
OMResult<std::any, std::string> checkArgCompat(std::any data, std::string type)
{
    auto typ = std::type_index(data.type());
    switch (hash_compile_time(type.c_str()))
    {
    case "int"_hash:
    case "byte"_hash:
    case "char"_hash:
    case "short"_hash:
    case "boolean"_hash:
        if (typ != std::type_index(typeid(int)))
        {
            return OMResult<std::any, std::string>::err(
                fmt::format("requested type {} but found data type non-int!", type));
        }
        break;
    case "void"_hash:
        return OMResult<std::any, std::string>::err(fmt::format("{} has no instance!", type));
    case "long"_hash:
        if (typ != std::type_index(typeid(int64_t)))
        {
            return OMResult<std::any, std::string>::err(
                fmt::format("requested type {} but found data type non-long!", type));
        }
        break;
    case "float"_hash:
        if (typ != std::type_index(typeid(float)))
        {
            return OMResult<std::any, std::string>::err(
                fmt::format("requested type {} but found data type non-float!", type));
        }
        break;

    case "double"_hash:
        if (typ != std::type_index(typeid(double)))
        {
            return OMResult<std::any, std::string>::err(
                fmt::format("requested type {} but found data type non-double! {}", type));
        }
        break;
    case "java/lang/String"_hash:
        if (typ != std::type_index(typeid(std::string)) && typ != std::type_index(typeid(const char *)))
        {
            return OMResult<std::any, std::string>::err(
                fmt::format("requested type {} but found data type non-string!", type));
        }
        break;
    }
    return OMResult<std::any, std::string>::ok(nullptr);
}
} // namespace openminecraft::vm::bytecode::descriptor