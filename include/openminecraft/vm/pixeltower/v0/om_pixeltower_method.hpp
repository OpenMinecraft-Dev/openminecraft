#ifndef OM_PIXELTOWER_METHOD_HPP
#define OM_PIXELTOWER_METHOD_HPP

#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
namespace openminecraft::vm::pixeltower::v0
{
struct OMMethod
{
    OMKlass *klass;
    const jbyte *name;
    const jbyte *desc;
    OMMethod *next;
    jint *lineNumberTable;
    AccessFlags accessFlags;
    jint maxLocals;
    jint maxStack;
    jint codeSize;
    jint args;
    uint8_t code[0];
    // code space
};
} // namespace openminecraft::vm::pixeltower::v0

#endif