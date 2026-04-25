#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "ffi.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_field.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <thread>

using namespace openminecraft::binary::hash;

namespace openminecraft::vm::elysia::executor
{
static int add(int a, int b)
{
    return a + b;
}

OMElysiaExecutorZero::OMElysiaExecutorZero(OMElysiaVirtualWorld *vw) : world(vw), logger("OMElysiaExecutorZero", this)
{
    auto functionPtr = (void (*)())&add;
    int argCount = 2;

    ffi_type **ffiArgTypes = (ffi_type **)malloc(sizeof(ffi_type *) * argCount);
    ffiArgTypes[0] = &ffi_type_sint;
    ffiArgTypes[1] = &ffi_type_sint;

    void **ffiArgs = (void **)malloc(sizeof(void *) * argCount);
    int a1 = 5;
    int a2 = 3;
    ffiArgs[0] = &a1;
    ffiArgs[1] = &a2;

    ffi_cif cif;
    ffi_type *returnFfiType = &ffi_type_sint;
    ffi_status ffiPrepStatus = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, (unsigned int)argCount, returnFfiType, ffiArgTypes);

    if (ffiPrepStatus == FFI_OK)
    {
        void *returnPtr = NULL;
        if (returnFfiType->size)
        {
            returnPtr = malloc(returnFfiType->size);
        }
        ffi_call(&cif, functionPtr, returnPtr, ffiArgs);

        int returnValue = *(int *)returnPtr;
        logger.info("ret: {}", returnValue);
    }
}
OMElysiaExecutorZero::~OMElysiaExecutorZero()
{
}

// Geopeila: Stack status when new function called
// (x: operator stack, y: local variable, o: frame metadata)
//
//  (Pre)    (Copy)   (New)
//
//
//          xxxxxxxx xxxxxxxx
//          xxxxxxxx xxxxxxxx
//          -------- oooooooo
// xxxxxxxx xxxxxxxx oooooooo
// xxxxxxxx xxxxxxxx oooooooo
// yyyyyyyy yyyyyyyy yyyyyyyy
// yyyyyyyy yyyyyyyy yyyyyyyy
// yyyyyyyy yyyyyyyy yyyyyyyy
// oooooooo oooooooo oooooooo
// oooooooo oooooooo oooooooo
// oooooooo oooooooo oooooooo
// ........ ........ ........
void OMElysiaExecutorZero::pushFrame(OMElysiaMethod *m)
{
    auto ll = argSlots(m->descriptor) + (m->isStatic() ? 0 : 1);
    auto tc = thisThread.metadata;

    std::memcpy(reinterpret_cast<void *>(tc->zero.stackPointer - sizeof(OMElysiaJavaFrame)),
                reinterpret_cast<void *>(tc->zero.stackPointer), ll * sizeof(void *));
    zeroStackPop(ll * sizeof(void *));

    auto frame = reinterpret_cast<OMElysiaJavaFrame *>(zeroStackAlloc(sizeof(OMElysiaJavaFrame)));
    zeroStackAlloc(ll * sizeof(void *));

    for (int i = ll; i < m->localLength; i++)
    {
        zeroStackPush<OMElysiaOop *>(nullptr);
    }
    frame->method = m;
    frame->returnAddr = tc->zero.pc;
    frame->caller = tc->zero.frame;
    tc->zero.frame = frame;
    tc->zero.pc = m->code;

    if (m->isNative())
    {
        switch (hash_compile_time(fmt::format("{}.{}", m->klass->name, m->name).c_str()))
        {
        case "java/lang/System.registerNatives"_hash:
            impl::Java_java_lang_System_registerNatives();
            popFrame();
            break;
        default:
            throw 0;
        }
    }
}

void OMElysiaExecutorZero::popFrame()
{
    auto tc = thisThread.metadata;

    // gino: klass init succeed
    if (std::strcmp(tc->zero.frame->method->name, "<clinit>") == 0)
    {
        tc->zero.frame->method->klass->toInstance()->clinitFinished = true;
    }

    tc->zero.stackPointer = reinterpret_cast<uintptr_t>(tc->zero.frame) + sizeof(OMElysiaJavaFrame);
    tc->zero.pc = tc->zero.frame->returnAddr;
    tc->zero.frame = tc->zero.frame->caller;
}

void OMElysiaExecutorZero::execute(OMElysiaMethod *m)
{
    auto tc = thisThread.metadata;
    if (!tc->threadInited)
    {
        tc->stackEnd = reinterpret_cast<uintptr_t>(mem::allocator::tracedMallocElysia(1024 * 1024));
        tc->stackStart = tc->stackEnd + 1024 * 1024 - sizeof(void *);
        tc->zero.stackPointer = tc->stackStart;

        tc->cleaner = [&]() { mem::allocator::tracedFreeElysia(reinterpret_cast<void *>(tc->stackEnd)); };
        tc->threadInited = true;
        tc->registerThread();

        logger.info("virtual stack: {}", (void *)tc->stackStart);
    }

    pushFrame(m);

    if (m->klass->isInstance() && !m->klass->toInstance()->clinitFinished)
    {
        auto l = m->klass->findMethod("<clinit>", "()V");
        if (l)
        {
            pushFrame(l);
        }
    }

#define CURRENT_KLASS tc->zero.frame->method->klass->toInstance()

    while (true)
    {
        switch (*tc->zero.pc)
        {
        case op_nop:
            ++tc->zero.pc;
            break;
        case op_aconst_null:
            ++tc->zero.pc;
            zeroStackPush<OMElysiaOop *>(nullptr);
            break;
#define op_iconst(n)                                                                                                   \
    case op_iconst_i(n):                                                                                               \
        zeroStackPush<jint>(n);                                                                                        \
        ++tc->zero.pc;                                                                                                 \
        break;

#define op_lconst(n)                                                                                                   \
    case op_lconst_l(n):                                                                                               \
        zeroStackPushW<jlong>(n);                                                                                      \
        ++tc->zero.pc;                                                                                                 \
        break;

#define op_fconst(n)                                                                                                   \
    case op_fconst_f(n):                                                                                               \
        zeroStackPush<jfloat>(n);                                                                                      \
        ++tc->zero.pc;                                                                                                 \
        break;

            op_iconst(-1);
            op_iconst(0);
            op_iconst(1);
            op_iconst(2);
            op_iconst(3);
            op_iconst(4);
            op_iconst(5);
            op_lconst(0);
            op_lconst(1);
            op_fconst(0);
            op_fconst(1);
            op_fconst(2);
        case op_bipush:
            ++tc->zero.pc;
            zeroStackPush<jint>(*tc->zero.pc);
            ++tc->zero.pc;
            break;
        case op_ldc: {
            auto ff = CURRENT_KLASS->constantPoolFetch(tc->zero.pc[1]);
            zeroStackPush(ff);
            tc->zero.pc += 2;
            break;
        }
        case op_iload_n(1):
            zeroStackPush(zeroStackLoadLocal<jint>(1));
            ++tc->zero.pc;
            break;
        case op_fload_n(2):
            zeroStackPush(zeroStackLoadLocal<jfloat>(2));
            ++tc->zero.pc;
            break;
        case op_aload_n(0):
            zeroStackPush(zeroStackLoadLocal<OMElysiaOop *>(0));
            ++tc->zero.pc;
            break;
        case op_istore_n(1):
            zeroStackSaveLocalPop<jint>(1);
            ++tc->zero.pc;
            break;
        case op_dup: {
            zeroStackPush(zeroStackPeekGet<OMElysiaOop *>());
            ++tc->zero.pc;
            break;
        }
        case op_iinc: {
            ++tc->zero.pc;
            auto slt = *tc->zero.pc;
            jint data = zeroStackLoadLocal<jint>(slt);
            ++tc->zero.pc;
            data += *tc->zero.pc;
            zeroStackSaveLocal(slt, data);
            ++tc->zero.pc;
            break;
        }
#define op_ifcmp(cond, op)                                                                                             \
    case op_if##cond: {                                                                                                \
        if (zeroStackPopGet<jint>() op 0)                                                                              \
        {                                                                                                              \
            tc->zero.pc += zeroCodeFetchArgs16p0();                                                                    \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            tc->zero.pc += 3;                                                                                          \
        }                                                                                                              \
        break;                                                                                                         \
    }
            op_ifcmp(eq, ==);
            op_ifcmp(ne, !=);
            op_ifcmp(lt, <);
            op_ifcmp(gt, >);
            op_ifcmp(ge, >=);
            op_ifcmp(le, <=);

#define op_ificmp(cond, op)                                                                                            \
    case op_if_icmp##cond: {                                                                                           \
        if (zeroStackPopGet<jint>() op zeroStackPopGet<jint>())                                                        \
        {                                                                                                              \
            tc->zero.pc += zeroCodeFetchArgs16p0();                                                                    \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            tc->zero.pc += 3;                                                                                          \
        }                                                                                                              \
        break;                                                                                                         \
    }
            op_ificmp(eq, ==);
            op_ificmp(ne, !=);
            op_ificmp(lt, <);
            op_ificmp(gt, >);
            op_ificmp(ge, >=);
            op_ificmp(le, <=);

#define op_fcmp(cond, opnan, op)                                                                                       \
    case op_fcmp##cond: {                                                                                              \
        auto value2 = zeroStackPopGet<jfloat>();                                                                       \
        auto value1 = zeroStackPopGet<jfloat>();                                                                       \
        if (value1 == NAN || value2 == NAN || value1 opnan value2)                                                     \
        {                                                                                                              \
            zeroStackPush<jint>(1);                                                                                    \
        }                                                                                                              \
        else if (value1 op value2)                                                                                     \
        {                                                                                                              \
            zeroStackPush<jint>(-1);                                                                                   \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            zeroStackPush<jint>(0);                                                                                    \
        }                                                                                                              \
        ++tc->zero.pc;                                                                                                 \
        break;                                                                                                         \
    }
            op_fcmp(g, >, <);
            op_fcmp(l, <, >);
#define op_dcmp(cond, opnan, op)                                                                                       \
    case op_dcmp##cond: {                                                                                              \
        auto value2 = zeroStackPopWGet<jdouble>();                                                                     \
        auto value1 = zeroStackPopWGet<jdouble>();                                                                     \
        if (value1 == NAN || value2 == NAN || value1 opnan value2)                                                     \
        {                                                                                                              \
            zeroStackPush<jint>(1);                                                                                    \
        }                                                                                                              \
        else if (value1 op value2)                                                                                     \
        {                                                                                                              \
            zeroStackPush<jint>(-1);                                                                                   \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            zeroStackPush<jint>(0);                                                                                    \
        }                                                                                                              \
        ++tc->zero.pc;                                                                                                 \
        break;                                                                                                         \
    }
            op_dcmp(g, >, <);
            op_dcmp(l, <, >);
        case op_return: {
            popFrame();
            break;
        }
        case op_putfield: {
            auto fld = CURRENT_KLASS->constantPoolFetch(zeroCodeFetchArgu16p0());
            zeroStackPopToField(reinterpret_cast<OMElysiaField *>(fld), world->oopManager.get());
            tc->zero.pc += 3;
            break;
        }
        case op_putstatic: {
            auto fld = CURRENT_KLASS->constantPoolFetch(zeroCodeFetchArgu16p0());
            zeroStackPopToStatic(reinterpret_cast<OMElysiaField *>(fld));
            tc->zero.pc += 3;
            break;
        }
        case op_invokestatic: {
            auto ff = CURRENT_KLASS->constantPoolFetch(zeroCodeFetchArgu16p0());
            tc->zero.pc += 3;
            pushFrame(reinterpret_cast<OMElysiaMethod *>(ff));
            break;
        }
        case op_invokespecial: {
            auto ff = CURRENT_KLASS->constantPoolFetch(zeroCodeFetchArgu16p0());
            tc->zero.pc += 3;
            pushFrame(reinterpret_cast<OMElysiaMethod *>(ff));
            break;
        }
        case op_new: {
            auto c = CURRENT_KLASS->constantPoolFetch(zeroCodeFetchArgu16p0());
            zeroStackPush(world->oopManager->allocateOop(reinterpret_cast<OMElysiaKlass *>(c)));
            tc->zero.pc += 3;
            break;
        }
        default:
        unk:
            while (true)
            {
                continue;
            }
        }
    }
}

uintptr_t zeroStackAlloc(uint64_t len)
{
    auto tc = thisThread.metadata;
    tc->zero.stackPointer = tc->zero.stackPointer - len;
    return tc->zero.stackPointer;
}

uintptr_t zeroStackPop(uint64_t len)
{
    auto tc = thisThread.metadata;
    auto result = tc->zero.stackPointer;
    tc->zero.stackPointer = tc->zero.stackPointer + len;
    return result;
}

uint16_t zeroCodeFetchArgu16p0()
{
    auto tc = thisThread.metadata;
    return static_cast<uint16_t>(tc->zero.pc[1] << 8) | tc->zero.pc[2];
}

int16_t zeroCodeFetchArgs16p0()
{
    auto tc = thisThread.metadata;
    return static_cast<int16_t>(tc->zero.pc[1] << 8) | tc->zero.pc[2];
}

void zeroStackPopToStatic(OMElysiaField *field)
{
    switch (*field->desc)
    {
    case 'J':
    case 'D':
        *reinterpret_cast<jlong *>(reinterpret_cast<uintptr_t>(field->klass->staticBlock) + field->offset) =
            zeroStackPopWGet<jlong>();
        break;
    default:
        *reinterpret_cast<jint *>(reinterpret_cast<uintptr_t>(field->klass->staticBlock) + field->offset) =
            zeroStackPopGet<jint>();
        break;
    }
}

void zeroStackPopToField(OMElysiaField *field, OMElysiaOopManager *oop)
{
    switch (*field->desc)
    {
    case 'J':
    case 'D': {
        auto pp = zeroStackPopWGet<jlong>();
        *reinterpret_cast<jlong *>(oop->oopAccessField(zeroStackPopGet<OMElysiaOop *>(), field->offset)) = pp;
        break;
    }
    default: {
        auto pp = zeroStackPopGet<jint>();
        *reinterpret_cast<jint *>(oop->oopAccessField(zeroStackPopGet<OMElysiaOop *>(), field->offset)) = pp;
        break;
    }
    }
}
} // namespace openminecraft::vm::elysia::executor
