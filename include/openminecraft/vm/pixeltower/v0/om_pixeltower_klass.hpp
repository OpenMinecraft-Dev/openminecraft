#ifndef OM_PIXELTOWER_KLASS_HPP
#define OM_PIXELTOWER_KLASS_HPP

#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include <cstdint>
#include <string>
namespace openminecraft::vm::pixeltower::v0
{
enum OMKlassKind : uint8_t
{
    Normal,
    Array
};

class OMKlass
{
    OMKlassKind kind;
    std::string name;
    OMKlass *superClass;
    jint accessFlags;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif