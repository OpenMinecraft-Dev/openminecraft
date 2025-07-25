#ifndef OM_PIXELTOWER_METHOD_HPP
#define OM_PIXELTOWER_METHOD_HPP

#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
namespace openminecraft::vm::pixeltower::v0
{
class OMMethod
{
    OMKlass *klass;
    jbyte *code;
    jint codeSize;
    jint *lineNumberTable;
    AccessFlags accessFlags;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif