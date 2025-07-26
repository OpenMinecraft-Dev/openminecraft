#ifndef OM_PIXELTOWER_OOP_HPP
#define OM_PIXELTOWER_OOP_HPP

#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"

namespace openminecraft::vm::pixeltower::v0
{
class OMKlass;
struct OMOOPDesc
{
    OMKlass *klass;
    jint mark;
    jbyte data[0];
    // object data area
};
struct OMOOPArrDesc
{
    OMKlass *klass;
    jint mark;
    jint length;
    jbyte data[0];
    // array data area
};
} // namespace openminecraft::vm::pixeltower::v0

#endif