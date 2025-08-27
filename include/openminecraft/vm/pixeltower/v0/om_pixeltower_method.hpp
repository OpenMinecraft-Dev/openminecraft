#ifndef OM_PIXELTOWER_METHOD_HPP
#define OM_PIXELTOWER_METHOD_HPP

#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include <unordered_map>
namespace openminecraft::vm::pixeltower::v0
{
struct OMMethodExceptionCaught
{
    jint begin;
    jint end;
    jint target;
    OMKlass *klass;
};

struct OMMethod
{
    OMKlass *klass;
    const jbyte *name;
    const jbyte *desc;
    std::unordered_map<jint, OMKlass *> *argCheck;
    std::unordered_map<jint, jint> *sourceMap;
    OMMethod *next;
    jint *lineNumberTable;
    AccessFlags accessFlags;
    jint maxLocals;
    jint maxStack;
    jint codeSize;
    jint args;
    std::vector<OMMethodExceptionCaught> *exceptionHandlers;
    uint8_t code[0];
    // code space
};
} // namespace openminecraft::vm::pixeltower::v0

#endif