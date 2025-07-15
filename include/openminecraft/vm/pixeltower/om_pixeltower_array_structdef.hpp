#ifndef OM_PIXELTOWER_ARRAY_STRUCTDEF_HPP
#define OM_PIXELTOWER_ARRAY_STRUCTDEF_HPP

#include "openminecraft/vm/pixeltower/om_pixeltower_class_structdef.hpp"
#include <cstdint>
namespace openminecraft::vm::pixeltower
{
enum OMArrayType : uint8_t
{
    Byte,
    Char,
    Short,
    Int,
    Long,
    Boolean,
    Double,
    Float,
    Reference
};

struct OMArrayHeader
{
    void *classifierPointer; // binded to a std::string which contains data "array"
    OMClass *classPointer;
    OMArrayType type;
    uint8_t dim;
    int length;
    // data section
};
} // namespace openminecraft::vm::pixeltower

#endif