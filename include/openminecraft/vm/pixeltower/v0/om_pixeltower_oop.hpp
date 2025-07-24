#ifndef OM_PIXELTOWER_OOP_HPP
#define OM_PIXELTOWER_OOP_HPP

#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"

namespace openminecraft::vm::pixeltower::v0
{
class OMOOPDesc
{
    jint mark;
    OMKlass *klass;
    // object data area
};
} // namespace openminecraft::vm::pixeltower::v0

#endif