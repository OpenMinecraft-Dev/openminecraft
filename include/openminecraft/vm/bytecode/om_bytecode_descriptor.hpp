#ifndef OM_BYTECODE_DESCRIPTOR_HPP
#define OM_BYTECODE_DESCRIPTOR_HPP

#include <string>
#include <utility>
#include <vector>

namespace openminecraft::vm::bytecode::descriptor
{
enum OMType
{
    Byte,
    Boolean,
    Char,
    Short,
    Int,
    Float,
    Long,
    Double,
    Array,
    Void,
    Reference
};
struct OMTypeDesc;
struct OMTypeDesc
{
    OMType type;
    std::string name;
    int depth;
    OMType subtype;
};

std::string restore(OMTypeDesc type);
OMTypeDesc decodeTypeTo(std::string raw, int *p);
std::pair<std::vector<OMTypeDesc>, OMTypeDesc> decodeSignatureTo(std::string raw, int *p);
} // namespace openminecraft::vm::bytecode::descriptor

#endif
