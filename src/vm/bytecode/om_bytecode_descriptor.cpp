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
        auto type = std::string(raw.substr(*p - 1, ends).c_str());
        auto d = OMResult<std::string, std::string>::ok(type);
        raw[ends] = ';';
        *p = ends + 1;
        return d;
    }
    case '[': {
        char begin = *p - 1;
        while (raw[*p] == '[')
        {
            *p = *p + 1;
        }
        char dim = *p - begin;
        auto sup = decodeType(raw, p);
        switch (sup.type)
        {
        case Ok:
            return OMResult<std::string, std::string>::ok(fmt::format("[{}{}", dim, sup.unwrap()));
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
} // namespace openminecraft::vm::bytecode::descriptor
