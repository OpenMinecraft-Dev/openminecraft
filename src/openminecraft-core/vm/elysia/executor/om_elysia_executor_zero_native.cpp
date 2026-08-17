#include "fmt/format.h"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_meta.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include <array>
#include <cstdint>
#include <ffi.h>
#include <variant>

using namespace openminecraft::binary::hash;

namespace openminecraft::vm::elysia::executor
{
OMElysiaNativeHandle *globalRefs = nullptr;
static void cleanupLocalRef()
{
    auto tc = thisThread.metadata;
    auto *current = reinterpret_cast<OMElysiaNativeHandle *>(tc->zero.frame ? tc->zero.frame->objectRefs : globalRefs);
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
auto OMElysiaExecutorZero::recordLocalRef(OMElysiaOop *oop) -> OMElysiaNativeHandle *
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

void OMElysiaExecutorZero::executeNative(OMElysiaMethod *m, bool isStatic, void *func, uint8_t **realpc,
                                         OMElysiaJavaFrame *frame)
{
    auto tc = thisThread.metadata;
    std::array<std::variant<jint, jbyte, jboolean, jshort, jchar, jfloat, jlong, jdouble, OMElysiaNativeHandle *,
                            OMElysiaJNIEnv *, OMElysiaKlass *>,
               258>
        rawargs;
    int rawargsize = 0;
#define appendArg(n)                                                                                                   \
    rawargs[rawargsize] = n;                                                                                           \
    ++rawargsize;

    // geopelia: arg #0, jnienv
    appendArg(&tc->interface);
    // rawargs.emplace_back(&tc->interface);

    std::array<uint8_t, 255> argTypes;
    int argCount;
    uint8_t returnType;
    descriptorTypes(m->cachedDescriptor, argTypes.data(), argCount, &returnType);
    auto argid = 0;

    // geopelia: arg #1, instance oop (non-static) or klass (static)
    if (!isStatic)
    {
        appendArg(recordLocalRef(zeroStackLoadLocal<OMElysiaOop *>(argid, frame)));
        ++argid;
    }
    else
    {
        appendArg(m->klass);
    }

    // geopelia: arg #2 and so on, fetch from the stack (if exists)
    for (int i = 0; i < argCount; i++)
    {
        switch (argTypes[i])
        {

#define ARGTYPE_CASE0(id, type)                                                                                        \
    case id:                                                                                                           \
        appendArg(zeroStackLoadLocal<type>(argid, frame));                                                             \
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
            appendArg(recordLocalRef(zeroStackLoadLocal<OMElysiaOop *>(argid, frame)));
            ++argid;
            break;
        case argTypeLong:
            appendArg(zeroStackLoadLocalW<jlong>(argid, frame));
            argid += 2;
            break;
        case argTypeDouble:
            appendArg(zeroStackLoadLocalW<jdouble>(argid, frame));
            argid += 2;
            break;
        default:
            throw std::logic_error("not supported yet!");
        }
    }

    void **argPointers = reinterpret_cast<void **>(zeroStackAlloc(sizeof(void *) * rawargsize));
    for (int i = 0; i < rawargsize; i++)
    {
#define PUSHARG(type)                                                                                                  \
    if (auto *p = std::get_if<type>(&rawargs[i]))                                                                      \
    {                                                                                                                  \
        argPointers[i] = p;                                                                                            \
    }
        PUSHARG(jbyte);
        PUSHARG(jboolean);
        PUSHARG(jshort);
        PUSHARG(jchar);
        PUSHARG(jint);
        PUSHARG(jfloat);
        PUSHARG(jlong);
        PUSHARG(jdouble);
        PUSHARG(OMElysiaNativeHandle *);
        PUSHARG(OMElysiaKlass *);
        PUSHARG(OMElysiaJNIEnv *);
    }

    if (!m->cifprepared)
    {
        m->nativeArgTypes =
            reinterpret_cast<ffi_type **>(mem::allocator::tracedMallocElysiaExternal(sizeof(void *) * rawargsize));

        for (int i = 0; i < rawargsize; i++)
        {
#define PUSHARG2(ffitype, type)                                                                                        \
    if (auto *p = std::get_if<type>(&rawargs[i]))                                                                      \
    {                                                                                                                  \
        m->nativeArgTypes[i] = &ffitype;                                                                               \
    }
            PUSHARG2(ffi_type_sint8, jbyte);
            PUSHARG2(ffi_type_uint8, jboolean);
            PUSHARG2(ffi_type_sint16, jshort);
            PUSHARG2(ffi_type_uint16, jchar);
            PUSHARG2(ffi_type_sint32, jint);
            PUSHARG2(ffi_type_float, jfloat);
            PUSHARG2(ffi_type_sint64, jlong);
            PUSHARG2(ffi_type_double, jdouble);
            PUSHARG2(ffi_type_pointer, OMElysiaNativeHandle *);
            PUSHARG2(ffi_type_pointer, OMElysiaKlass *);
            PUSHARG2(ffi_type_pointer, OMElysiaJNIEnv *);
        }

        // ffi_type *retType;
        switch (returnType)
        {
        case argTypeVoid:
            m->nativeReturnType = &ffi_type_void;
            break;
        case argTypeReference:
        case argTypeArray:
            m->nativeReturnType = &ffi_type_pointer;
            break;
        case argTypeInt:
        case argTypeShort:
        case argTypeChar:
        case argTypeBoolean:
        case argTypeByte:
            m->nativeReturnType = &ffi_type_sint32;
            break;
        case argTypeFloat:
            m->nativeReturnType = &ffi_type_float;
            break;
        case argTypeLong:
            m->nativeReturnType = &ffi_type_sint64;
            break;
        case argTypeDouble:
            m->nativeReturnType = &ffi_type_double;
            break;
        default:
            m->nativeReturnType = &ffi_type_void;
            break;
        }
        ffi_prep_cif(&m->cif, FFI_DEFAULT_ABI, rawargsize, m->nativeReturnType, m->nativeArgTypes);
        m->cifprepared = true;
    }

    uint64_t retValueReal = 0;
    void *retValue = &retValueReal;

    execWithState(InsideNative,
                  [&]() -> void { ffi_call(&m->cif, reinterpret_cast<void (*)()>(func), retValue, argPointers); });

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
                free(vv);
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

void OMElysiaExecutorZero::executeNativeLink(uint8_t **realpc, OMElysiaJavaFrame *frame)
{
    auto tc = thisThread.metadata;
    auto mm = tc->zero.frame->method;

    if (mm->code)
    {
        executeNative(mm, mm->isStatic(), mm->code, realpc, frame);
        return;
    }

    for (int i = 0; i < mm->klass->nativeMethodCount; i++)
    {
        auto &nm = mm->klass->nativeMethods[i];
        if (mm->isSame(&nm))
        {
            mm->code = reinterpret_cast<uint8_t *>(nm.funcPtr);
            executeNative(mm, mm->isStatic(), nm.funcPtr, realpc, frame);
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
        mm->code = reinterpret_cast<uint8_t *>(elysium->nativeFuncMap[ffm]);
        executeNative(mm, mm->isStatic(), elysium->nativeFuncMap[ffm], realpc, frame);
        return;
    }

    throw std::logic_error("not implemented: " + fmt::format("{}.{}{}", mm->klass->name, mm->name, mm->descriptor));
}
} // namespace openminecraft::vm::elysia::executor
