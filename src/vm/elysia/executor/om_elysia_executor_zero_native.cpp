#include "fmt/format.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_meta.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include <cstdint>
#include <ffi.h>
#include <iostream>
#include <variant>

using namespace openminecraft::binary::hash;

namespace openminecraft::vm::elysia::executor
{
OMElysiaNativeHandle *globalRefs = nullptr;
static void cleanupLocalRef()
{
    auto tc = thisThread.metadata;
    OMElysiaNativeHandle *current =
        reinterpret_cast<OMElysiaNativeHandle *>(tc->zero.frame ? tc->zero.frame->objectRefs : globalRefs);
    while (current)
    {
        auto oldCurr = current->next;
        mem::allocator::tracedFreeElysia(current);
        current = oldCurr;
    }

    if (tc->zero.frame)
    {
        tc->zero.frame->objectRefs = nullptr;
    }
    else
    {
        globalRefs = nullptr;
    }
}
OMElysiaNativeHandle *OMElysiaExecutorZero::recordLocalRef(OMElysiaOop *oop)
{
    if (!oop)
    {
        return nullptr;
    }
    auto tc = thisThread.metadata;

    auto oldnode = reinterpret_cast<OMElysiaNativeHandle *>(tc->zero.frame ? tc->zero.frame->objectRefs : globalRefs);
    auto newnode =
        reinterpret_cast<OMElysiaNativeHandle *>(mem::allocator::tracedCallocElysia(1, sizeof(OMElysiaNativeHandle)));
    newnode->next = oldnode;
    newnode->object = oop;
    if (tc->zero.frame)
    {
        tc->zero.frame->objectRefs = newnode;
    }
    else
    {
        globalRefs = newnode;
    }
    oldnode = newnode;

    return oldnode;
}

void OMElysiaExecutorZero::executeNative(char *descriptor, bool isStatic, void *func, uint8_t **realpc)
{
    auto tc = thisThread.metadata;
    std::vector<std::variant<jint, jbyte, jboolean, jshort, jchar, jfloat, jlong, jdouble, OMElysiaNativeHandle *,
                             OMElysiaJNIEnv *, OMElysiaKlass *>>
        rawargs;

    // geopelia: arg #0, jnienv
    rawargs.push_back(&tc->interface);

    uint8_t argTypes[255];
    int argCount;
    uint8_t returnType;
    descriptorTypes(descriptor, argTypes, argCount, &returnType);
    auto argid = 0;

    // geopelia: arg #1, instance oop (non-static) or klass (static)
    if (!isStatic)
    {
        rawargs.push_back(recordLocalRef(zeroStackLoadLocal<OMElysiaOop *>(argid)));
        ++argid;
    }
    else
    {
        rawargs.push_back(thisThread.metadata->zero.frame->method->klass);
    }

    // geopelia: arg #2 and so on, fetch from the stack (if exists)
    for (int i = 0; i < argCount; i++)
    {
        switch (argTypes[i])
        {

#define ARGTYPE_CASE0(id, type)                                                                                        \
    case id:                                                                                                           \
        rawargs.push_back(zeroStackLoadLocal<type>(argid));                                                            \
        ++argid;                                                                                                       \
        break;

            ARGTYPE_CASE0(argTypeByte, jbyte);
            ARGTYPE_CASE0(argTypeBoolean, jboolean);
            ARGTYPE_CASE0(argTypeShort, jshort);
            ARGTYPE_CASE0(argTypeChar, jchar);
            ARGTYPE_CASE0(argTypeInt, jint);
            ARGTYPE_CASE0(argTypeFloat, jfloat);
        case argTypeReference:
        case argTypeArray:
            rawargs.push_back(recordLocalRef(zeroStackLoadLocal<OMElysiaOop *>(argid)));
            ++argid;
            break;
        case argTypeLong:
            rawargs.push_back(zeroStackLoadLocalW<jlong>(argid));
            argid += 2;
            break;
        case argTypeDouble:
            rawargs.push_back(zeroStackLoadLocalW<jdouble>(argid));
            argid += 2;
            break;
        default:
            throw std::logic_error("not supported yet!");
        }
    }

    void **argPointers = reinterpret_cast<void **>(zeroStackAlloc(sizeof(void *) * rawargs.size()));
    std::vector<ffi_type *> rawargtypes;
    rawargtypes.reserve(rawargs.size());

    for (int i = 0; i < rawargs.size(); i++)
    {
#define PUSHARG(ffitype, type)                                                                                         \
    if (auto *p = std::get_if<type>(&rawargs[i]))                                                                      \
    {                                                                                                                  \
        argPointers[i] = p;                                                                                            \
        rawargtypes.push_back(&ffitype);                                                                               \
    }
        PUSHARG(ffi_type_sint8, jbyte);
        PUSHARG(ffi_type_uint8, jboolean);
        PUSHARG(ffi_type_sint16, jshort);
        PUSHARG(ffi_type_uint16, jchar);
        PUSHARG(ffi_type_sint32, jint);
        PUSHARG(ffi_type_float, jfloat);
        PUSHARG(ffi_type_sint64, jlong);
        PUSHARG(ffi_type_double, jdouble);
        PUSHARG(ffi_type_pointer, OMElysiaNativeHandle *);
        PUSHARG(ffi_type_pointer, OMElysiaKlass *);
        PUSHARG(ffi_type_pointer, OMElysiaJNIEnv *);
    }

    ffi_type *retType;
    switch (returnType)
    {
    case argTypeVoid:
        retType = &ffi_type_void;
        break;
    case argTypeReference:
    case argTypeArray:
        retType = &ffi_type_pointer;
        break;
    case argTypeInt:
    case argTypeShort:
    case argTypeChar:
    case argTypeBoolean:
    case argTypeByte:
        retType = &ffi_type_sint32;
        break;
    case argTypeFloat:
        retType = &ffi_type_float;
        break;
    case argTypeLong:
        retType = &ffi_type_sint64;
        break;
    case argTypeDouble:
        retType = &ffi_type_double;
        break;
    default:
        retType = &ffi_type_void;
        break;
    }

    ffi_cif cif;
    ffi_status ffiPrepStatus = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, rawargtypes.size(), retType, rawargtypes.data());
    auto retSiz = std::max(sizeof(void *), retType->size);
    void *retValue = reinterpret_cast<void *>(zeroStackAlloc(retSiz));
    std::memset(retValue, 0x00, retSiz);

    if (ffiPrepStatus == FFI_OK)
    {
        execWithState(InsideNative,
                      [&]() { ffi_call(&cif, reinterpret_cast<void (*)()>(func), retValue, argPointers); });
    }

    switch (returnType)
    {
    case argTypeVoid:
        popFrame(realpc);
        break;
    case argTypeArray:
    case argTypeReference: {
        auto vv = *reinterpret_cast<OMElysiaNativeHandle **>(retValue);
        popFrame(realpc);
        zeroStackPush(handleFetch(vv));
        if (vv)
        {
            if (vv->next == vv)
            {
                mem::allocator::tracedFreeElysia(vv);
            }
        }
        break;
    }
    case argTypeChar: {
        auto data = *reinterpret_cast<jchar *>(retValue);
        popFrame(realpc);
        zeroStackPush(data);
        break;
    }
    case argTypeBoolean: {
        auto data = *reinterpret_cast<jboolean *>(retValue);
        popFrame(realpc);
        zeroStackPush(data);
        break;
    }
    case argTypeByte: {
        auto data = *reinterpret_cast<jbyte *>(retValue);
        popFrame(realpc);
        zeroStackPush(data);
        break;
    }
    case argTypeShort: {
        auto data = *reinterpret_cast<jshort *>(retValue);
        popFrame(realpc);
        zeroStackPush(data);
        break;
    }
    case argTypeInt: {
        auto data = *reinterpret_cast<jint *>(retValue);
        popFrame(realpc);
        zeroStackPush(data);
        break;
    }
    case argTypeFloat: {
        auto data = *reinterpret_cast<jfloat *>(retValue);
        popFrame(realpc);
        zeroStackPush(data);
        break;
    }
    case argTypeLong: {
        auto data = *reinterpret_cast<jlong *>(retValue);
        popFrame(realpc);
        zeroStackPushW(data);
        break;
    }
    case argTypeDouble: {
        auto data = *reinterpret_cast<jdouble *>(retValue);
        popFrame(realpc);
        zeroStackPushW(data);
        break;
    }
    default:
        popFrame(realpc);
        break;
    }

    cleanupLocalRef();
}

void OMElysiaExecutorZero::executeNativeLink(uint8_t **realpc)
{
    auto tc = thisThread.metadata;
    auto mm = tc->zero.frame->method;

    for (int i = 0; i < mm->klass->nativeMethodCount; i++)
    {
        auto &nm = mm->klass->nativeMethods[i];
        if (mm->isSame(&nm))
        {
            executeNative(mm->descriptor, mm->isStatic(), nm.funcPtr, realpc);
            return;
        }
    }

    auto ffm = fmt::format("Java_{}_{}", mm->klass->name, mm->name);
    for (auto &c : ffm)
    {
        if (c == '/')
        {
            c = '_';
        }
    }

    if (elysium->nativeFuncMap.count(ffm))
    {
        executeNative(mm->descriptor, mm->isStatic(), elysium->nativeFuncMap[ffm], realpc);
        return;
    }

    throw std::logic_error("not implemented: " + fmt::format("{}.{}{}", mm->klass->name, mm->name, mm->descriptor));
}
} // namespace openminecraft::vm::elysia::executor
