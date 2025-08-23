#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include <variant>
#include <vector>

namespace openminecraft::vm::bytecode::descriptor
{
std::string restore(OMTypeDesc type)
{
    switch (type.type)
    {
    case Byte:
        return "B";
    case Boolean:
        return "Z";
    case Char:
        return "C";
    case Short:
        return "S";
    case Int:
        return "I";
    case Float:
        return "F";
    case Long:
        return "J";
    case Double:
        return "D";
    case Array: {
        std::string ii = "";
        for (int i = 0; i < type.depth; i++)
        {
            ii += "[";
        }
        ii += restore({type.subtype, type.name});
        return ii;
    }
    case Reference:
        return type.name;
    case Void:
    default:
        return "V";
    }
}
std::pair<std::vector<OMTypeDesc>, OMTypeDesc> decodeSignatureTo(std::string raw, int *p)
{
    if (raw[*p] != '(')
    {
        throw err::OMValidationError{err::Unknown, "not a method signature", raw};
    }
    *p = *p + 1;
    std::vector<OMTypeDesc> args;
    while (raw[*p] != ')')
    {
        args.push_back(decodeTypeTo(raw, p));
    }
    *p = *p + 1;

    auto ret = decodeTypeTo(raw, p);
    return std::pair<std::vector<OMTypeDesc>, OMTypeDesc>(args, ret);
}
OMTypeDesc decodeTypeTo(std::string raw, int *p)
{
    if (*p >= raw.length())
    {
        throw err::OMValidationError{err::Unknown, "type string ends!", raw};
    }
    *p = *p + 1;
    switch (raw[*p - 1])
    {
    case 'V':
        return {Void};
    case 'B':
        return {Byte};
    case 'C':
        return {Char};
    case 'D':
        return {Double};
    case 'F':
        return {Float};
    case 'I':
        return {Int};
    case 'J':
        return {Long};
    case 'S':
        return {Short};
    case 'Z':
        return {Boolean};
    case 'L': {
        auto ends = raw.find_first_of(';', *p - 1) - *p;
        if (ends == std::variant_npos)
        {
            throw err::OMValidationError{err::Unknown, "nonstop object type", raw};
        }
        // inplace split
        auto type = std::string(raw.c_str()).substr(*p, ends);
        OMTypeDesc d = {Reference, type};
        *p += ends + 1;
        return d;
    }
    case '[': {
        char begin = *p - 1;
        while (raw[*p] == '[')
        {
            *p = *p + 1;
        }
        char dim = *p - begin;
        auto sup = decodeTypeTo(raw, p);
        return {Array, sup.name, dim, sup.type};
    }
    }
    *p = *p - 1;
    throw err::OMValidationError{err::Unknown, "not recognized", raw};
}
} // namespace openminecraft::vm::bytecode::descriptor
