#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include <ffi.h>
#include <variant>

using namespace openminecraft::binary::hash;

namespace openminecraft::vm::elysia::executor
{
static void cleanupLocalRef()
{
    auto tc = thisThread.metadata;
    OMElysiaNativeHandle *current = reinterpret_cast<OMElysiaNativeHandle *>(tc->zero.frame->flag);
    while (current)
    {
        auto oldCurr = current->next;
        mem::allocator::tracedFreeElysia(current);
        current = oldCurr;
    }
    tc->zero.frame->flag = nullptr;
}
OMElysiaNativeHandle *OMElysiaExecutorZero::recordLocalRef(OMElysiaOop *oop)
{
    if (!oop)
    {
        return nullptr;
    }
    auto tc = thisThread.metadata;
    auto oldnode = reinterpret_cast<OMElysiaNativeHandle *>(tc->zero.frame->flag);
    if (!oldnode->object)
    {
        oldnode->object = oop;
    }
    else
    {
        auto newnode = reinterpret_cast<OMElysiaNativeHandle *>(
            mem::allocator::tracedCallocElysia(1, sizeof(OMElysiaNativeHandle)));
        newnode->next = oldnode;
        newnode->object = oop;
        tc->zero.frame->flag = newnode;
        oldnode = newnode;
    }

    return oldnode;
}
void OMElysiaExecutorZero::executeNative(char *descriptor, bool isStatic, void *func)
{
    auto tc = thisThread.metadata;
    tc->zero.frame->flag = mem::allocator::tracedCallocElysia(1, sizeof(OMElysiaNativeHandle));

    std::vector<std::variant<jint, jbyte, jboolean, jshort, jchar, jfloat, jlong, jdouble, OMElysiaNativeHandle *>>
        rawargs;
    std::vector<ffi_type *> rawargtypes;

    uint8_t argTypes[255];
    int argCount;
    uint8_t returnType;
    argDescriptorParse(descriptor, argTypes, argCount, &returnType);
    auto argid = 0;

    if (!isStatic)
    {
        rawargs.push_back(recordLocalRef(zeroStackLoadLocal<OMElysiaOop *>(argid)));
        rawargtypes.push_back(&ffi_type_pointer);
        ++argid;
    }

    for (int i = 0; i < argCount; i++)
    {
        switch (argTypes[i])
        {

#define ARGTYPE_CASE(id, type, ffitype)                                                                                \
    case id:                                                                                                           \
        rawargs.push_back(zeroStackLoadLocal<type>(argid));                                                            \
        rawargtypes.push_back(ffitype);                                                                                \
        ++argid;                                                                                                       \
        break;

            ARGTYPE_CASE(argTypeByte, jbyte, &ffi_type_uint8);
            ARGTYPE_CASE(argTypeBoolean, jboolean, &ffi_type_uint8);
            ARGTYPE_CASE(argTypeShort, jshort, &ffi_type_sint16);
            ARGTYPE_CASE(argTypeChar, jchar, &ffi_type_uint16);
            ARGTYPE_CASE(argTypeInt, jint, &ffi_type_sint32);
            ARGTYPE_CASE(argTypeFloat, jfloat, &ffi_type_float);
        case argTypeReference:
            rawargs.push_back(recordLocalRef(zeroStackLoadLocal<OMElysiaOop *>(argid)));
            rawargtypes.push_back(&ffi_type_pointer);
            ++argid;
            break;
        default:
            throw std::logic_error("not supported yet!");
        }
    }

    void **argPointers =
        reinterpret_cast<void **>(zeroStackAlloc(sizeof(void *) * (rawargs.size() + (isStatic ? 1 : 0))));

    auto pp = &thisThread.metadata->interface;
    argPointers[0] = &pp;
    rawargtypes.insert(rawargtypes.begin(), &ffi_type_pointer);

    int argbegin;
    if (isStatic)
    {
        argPointers[1] = &thisThread.metadata->zero.frame->method->klass;
        rawargtypes.insert(rawargtypes.begin(), &ffi_type_pointer);
        argbegin = 2;
    }
    else
    {
        argbegin = 1;
    }

    for (int i = 0; i < rawargs.size(); i++)
    {
        auto &r = argPointers[argbegin + i];

#define try_type(type)                                                                                                 \
    if (auto *p = std::get_if<type>(&rawargs[i]))                                                                      \
    {                                                                                                                  \
        r = p;                                                                                                         \
    }
        try_type(jint);
        try_type(jfloat);
        try_type(jboolean);
        try_type(jbyte);
        try_type(jchar);
        try_type(jshort);
        try_type(jlong);
        try_type(jdouble);
        try_type(OMElysiaNativeHandle *);
    }

    ffi_type *retType;
    switch (returnType)
    {
    case argTypeVoid:
        retType = &ffi_type_void;
        break;
    case argTypeReference:
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
    void *retValue = reinterpret_cast<void *>(zeroStackAlloc(std::max(sizeof(void *), retType->size)));

    if (ffiPrepStatus == FFI_OK)
    {
        thisThread.switchState(InsideNative);
        ffi_call(&cif, reinterpret_cast<void (*)()>(func), retValue, argPointers);
        thisThread.switchState(InsideVM);
    }

    cleanupLocalRef();
    popFrame();

    switch (returnType)
    {
    case argTypeVoid:
        break;
    case argTypeReference:
        zeroStackPush(*reinterpret_cast<OMElysiaOop **>(retValue));
        break;
    case argTypeInt:
    case argTypeShort:
    case argTypeChar:
    case argTypeBoolean:
    case argTypeByte:
        zeroStackPush(*reinterpret_cast<jint *>(retValue));
        break;
    case argTypeFloat:
        zeroStackPush(*reinterpret_cast<jfloat *>(retValue));
        break;
    case argTypeLong:
        zeroStackPushW(*reinterpret_cast<jlong *>(retValue));
        break;
    case argTypeDouble:
        zeroStackPushW(*reinterpret_cast<jdouble *>(retValue));
        break;
    default:
        break;
    }
}

void OMElysiaExecutorZero::executeNativeLink()
{
    thisThread.switchState(InsideVM);
    auto tc = thisThread.metadata;
    auto mm = tc->zero.frame->method;
    // TODO: use dynamic loading
    switch (hash_compile_time(fmt::format("{}.{}", mm->klass->name, mm->name).c_str()))
    {
    case "java/lang/System.registerNatives"_hash:
        executeNative(mm->descriptor, mm->isStatic(), (void *)&impl::Java_java_lang_System_registerNatives);
        break;
    case "java/lang/Object.registerNatives"_hash:
        executeNative(mm->descriptor, mm->isStatic(), (void *)&impl::Java_java_lang_Object_registerNatives);
        break;
    case "java/lang/Class.registerNatives"_hash:
        executeNative(mm->descriptor, mm->isStatic(), (void *)&impl::Java_java_lang_Class_registerNatives);
        break;
    default:
        for (int i = 0; i < mm->klass->nativeMethodCount; i++)
        {
            auto &nm = mm->klass->nativeMethods[i];
            if (mm->isSame(&nm))
            {
                executeNative(mm->descriptor, mm->isStatic(), nm.funcPtr);
                return;
            }
        }
        throw std::logic_error("not implemented: " + fmt::format("{}.{}{}", mm->klass->name, mm->name, mm->descriptor));
    }
}
} // namespace openminecraft::vm::elysia::executor
