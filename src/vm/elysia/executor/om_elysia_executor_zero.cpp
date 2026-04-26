#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "ffi.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_field.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
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

#define op_dconst(n)                                                                                                   \
    case op_dconst_d(n):                                                                                               \
        zeroStackPushW<jdouble>(n);                                                                                    \
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
            op_dconst(0);
            op_dconst(1);
        case op_bipush:
            ++tc->zero.pc;
            zeroStackPush<jint>(*tc->zero.pc);
            ++tc->zero.pc;
            break;
        case op_sipush:
            zeroStackPush<jint>(zeroCodeFetchArgu16p0());
            tc->zero.pc += 3;
            break;
        case op_ldc: {
            auto ff = CURRENT_KLASS->constantPoolFetch(tc->zero.pc[1]);
            zeroStackPush(ff);
            tc->zero.pc += 2;
            break;
        }
#define op_iloadc(n)                                                                                                   \
    case op_iload_n(n):                                                                                                \
        zeroStackPush(zeroStackLoadLocal<jint>(n));                                                                    \
        ++tc->zero.pc;                                                                                                 \
        break;
            op_iloadc(0);
            op_iloadc(1);
            op_iloadc(2);
            op_iloadc(3);
#define op_floadc(n)                                                                                                   \
    case op_fload_n(n):                                                                                                \
        zeroStackPush(zeroStackLoadLocal<jfloat>(n));                                                                  \
        ++tc->zero.pc;                                                                                                 \
        break;
            op_floadc(0);
            op_floadc(1);
            op_floadc(2);
            op_floadc(3);
#define op_aloadc(n)                                                                                                   \
    case op_aload_n(n):                                                                                                \
        zeroStackPush(zeroStackLoadLocal<OMElysiaOop *>(n));                                                           \
        ++tc->zero.pc;                                                                                                 \
        break;
            op_aloadc(0);
            op_aloadc(1);
            op_aloadc(2);
            op_aloadc(3);

        case op_istore_n(1):
            zeroStackSaveLocalPop<jint>(1);
            ++tc->zero.pc;
            break;
        case op_dup: {
            zeroStackPush(zeroStackPeekGet<OMElysiaOop *>());
            ++tc->zero.pc;
            break;
        }

#define op_calc(op, fetch, psh, oprt)                                                                                  \
    case op_##op: {                                                                                                    \
        auto value2 = fetch();                                                                                         \
        auto value1 = fetch();                                                                                         \
        psh(value1 oprt value2);                                                                                       \
        ++tc->zero.pc;                                                                                                 \
        break;                                                                                                         \
    }
            op_calc(iadd, zeroStackPopGet<jint>, zeroStackPush, +);
            op_calc(ladd, zeroStackPopWGet<jlong>, zeroStackPushW, +);
            op_calc(fadd, zeroStackPopGet<jfloat>, zeroStackPush, +);
            op_calc(dadd, zeroStackPopWGet<jdouble>, zeroStackPushW, +);
            op_calc(isub, zeroStackPopGet<jint>, zeroStackPush, -);
            op_calc(lsub, zeroStackPopWGet<jlong>, zeroStackPushW, -);
            op_calc(fsub, zeroStackPopGet<jfloat>, zeroStackPush, -);
            op_calc(dsub, zeroStackPopWGet<jdouble>, zeroStackPushW, -);
            op_calc(imul, zeroStackPopGet<jint>, zeroStackPush, *);
            op_calc(lmul, zeroStackPopWGet<jlong>, zeroStackPushW, *);
            op_calc(fmul, zeroStackPopGet<jfloat>, zeroStackPush, *);
            op_calc(dmul, zeroStackPopWGet<jdouble>, zeroStackPushW, *);
            op_calc(idiv, zeroStackPopGet<jint>, zeroStackPush, /);
            op_calc(ldiv, zeroStackPopWGet<jlong>, zeroStackPushW, /);
            op_calc(fdiv, zeroStackPopGet<jfloat>, zeroStackPush, /);
            op_calc(ddiv, zeroStackPopWGet<jdouble>, zeroStackPushW, /);

#define op_calcrem(op, fetch, psh)                                                                                     \
    case op_##op: {                                                                                                    \
        auto value2 = fetch();                                                                                         \
        auto value1 = fetch();                                                                                         \
        psh(value1 - (value1 / value2) * value2);                                                                      \
        ++tc->zero.pc;                                                                                                 \
        break;                                                                                                         \
    }
            op_calcrem(irem, zeroStackPopGet<jint>, zeroStackPush);
            op_calcrem(lrem, zeroStackPopWGet<jlong>, zeroStackPushW);
            op_calcrem(frem, zeroStackPopGet<jfloat>, zeroStackPush);
            op_calcrem(drem, zeroStackPopWGet<jdouble>, zeroStackPushW);

#define op_calcneg(op, fetch, psh)                                                                                     \
    case op_##op: {                                                                                                    \
        psh(-fetch());                                                                                                 \
        ++tc->zero.pc;                                                                                                 \
        break;                                                                                                         \
    }
            op_calcneg(ineg, zeroStackPopGet<jint>, zeroStackPush);
            op_calcneg(lneg, zeroStackPopWGet<jlong>, zeroStackPushW);
            op_calcneg(fneg, zeroStackPopGet<jfloat>, zeroStackPush);
            op_calcneg(dneg, zeroStackPopWGet<jdouble>, zeroStackPushW);
            op_calc(ishl, zeroStackPopGet<jint>, zeroStackPush, <<);
            op_calc(lshl, zeroStackPopWGet<jlong>, zeroStackPushW, <<);
            op_calc(ishr, zeroStackPopGet<jint>, zeroStackPush, >>);
            op_calc(lshr, zeroStackPopWGet<jlong>, zeroStackPushW, >>);
            // TODO: need to check bounds
            op_calc(iushr, zeroStackPopGet<jint>, zeroStackPush, >>);
            op_calc(lushr, zeroStackPopWGet<jlong>, zeroStackPushW, >>);

            op_calc(iand, zeroStackPopGet<jint>, zeroStackPush, &);
            op_calc(land, zeroStackPopWGet<jlong>, zeroStackPushW, &);
            op_calc(ior, zeroStackPopGet<jint>, zeroStackPush, |);
            op_calc(lor, zeroStackPopWGet<jlong>, zeroStackPushW, |);
            op_calc(ixor, zeroStackPopGet<jint>, zeroStackPush, ^);
            op_calc(lxor, zeroStackPopWGet<jlong>, zeroStackPushW, ^);

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
#define op_conv(op, target, targettype, source, sourcetype)                                                            \
    case op_##op: {                                                                                                    \
        target(static_cast<targettype>(source<sourcetype>()));                                                         \
        ++tc->zero.pc;                                                                                                 \
        break;                                                                                                         \
    }
            op_conv(i2l, zeroStackPushW, jlong, zeroStackPopGet, jint);
            op_conv(i2f, zeroStackPush, jfloat, zeroStackPopGet, jint);
            op_conv(i2d, zeroStackPushW, jdouble, zeroStackPopGet, jint);
            op_conv(l2i, zeroStackPush, jint, zeroStackPopWGet, jlong);
            op_conv(l2f, zeroStackPush, jfloat, zeroStackPopWGet, jlong);
            op_conv(l2d, zeroStackPushW, jdouble, zeroStackPopWGet, jlong);
            op_conv(f2i, zeroStackPush, jint, zeroStackPopGet, jfloat);
            op_conv(f2l, zeroStackPushW, jlong, zeroStackPopGet, jfloat);
            op_conv(d2i, zeroStackPush, jint, zeroStackPopWGet, jdouble);
            op_conv(f2d, zeroStackPushW, double, zeroStackPopGet, jfloat);
            op_conv(d2l, zeroStackPushW, jlong, zeroStackPopWGet, jdouble);
            op_conv(d2f, zeroStackPush, jfloat, zeroStackPopWGet, jdouble);
            op_conv(i2b, zeroStackPush, jbyte, zeroStackPopGet, jint);
            op_conv(i2c, zeroStackPush, jchar, zeroStackPopGet, jint);
            op_conv(i2s, zeroStackPush, jshort, zeroStackPopGet, jint);
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
        case op_goto: {
            tc->zero.pc += zeroCodeFetchArgs16p0();
            break;
        }
        case op_return: {
            popFrame();
            break;
        }
        case op_ireturn: {
            auto pp = zeroStackPopGet<jint>();
            popFrame();
            zeroStackPush(pp);
            break;
        }
        case op_freturn: {
            auto pp = zeroStackPopGet<jfloat>();
            popFrame();
            zeroStackPush(pp);
            break;
        }
        case op_putfield: {
            auto fld = CURRENT_KLASS->constantPoolFetch(zeroCodeFetchArgu16p0());
            zeroStackPopToField(reinterpret_cast<OMElysiaField *>(fld), world->oopManager.get(), world);
            tc->zero.pc += 3;
            break;
        }
        case op_putstatic: {
            auto fld = CURRENT_KLASS->constantPoolFetch(zeroCodeFetchArgu16p0());
            zeroStackPopToStatic(reinterpret_cast<OMElysiaField *>(fld), world);
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
        case op_newarray: {
            std::string kn;
            switch (tc->zero.pc[1])
            {
            case 4:
                kn = "[Z";
                break;
            case 5:
                kn = "[C";
                break;
            case 6:
                kn = "[F";
                break;
            case 7:
                kn = "[D";
                break;
            case 8:
                kn = "[B";
                break;
            case 9:
                kn = "[S";
                break;
            case 10:
                kn = "[I";
                break;
            case 11:
                kn = "[J";
                break;
            default:
                break;
            }

            auto arr =
                world->oopManager->allocateArr(world->klassLoader->findClass(kn)->toArray(), zeroStackPopGet<jint>());
            zeroStackPush(arr);
            tc->zero.pc += 2;
            break;
        }
        case op_anewarray: {
            auto c = CURRENT_KLASS->constantPoolFetch(zeroCodeFetchArgu16p0());
            auto arrcls = buildArray(reinterpret_cast<OMElysiaKlass *>(c)->name);
            auto klass = world->klassLoader->findClass(arrcls);
            if (!klass)
            {
                world->klassLoader->constructArrayClass(reinterpret_cast<OMElysiaKlass *>(c));
                klass = world->klassLoader->findClass(arrcls);
            }
            auto arr = world->oopManager->allocateArr(klass->toArray(), zeroStackPopGet<jint>());
            zeroStackPush(arr);
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

void zeroStackPopToStatic(OMElysiaField *field, OMElysiaVirtualWorld *world)
{
    switch (*field->desc)
    {
    case 'J':
    case 'D':
        *reinterpret_cast<jlong *>(reinterpret_cast<uintptr_t>(field->klass->staticBlock) + field->offset) =
            zeroStackPopWGet<jlong>();
        break;
    case 'L':
    case '[': {
        auto pp = zeroStackPopGet<OMElysiaOop *>();
        if (world->mainHeap.enablePtrCompress())
        {
            *reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(field->klass->staticBlock) + field->offset) =
                world->mainHeap.compress(pp);
        }
        else
        {
            *reinterpret_cast<OMElysiaOop **>(reinterpret_cast<uintptr_t>(field->klass->staticBlock) + field->offset) =
                pp;
        }
        break;
    }
    default:
        *reinterpret_cast<jint *>(reinterpret_cast<uintptr_t>(field->klass->staticBlock) + field->offset) =
            zeroStackPopGet<jint>();
        break;
    }
}

void zeroStackPopToField(OMElysiaField *field, OMElysiaOopManager *oop, OMElysiaVirtualWorld *world)
{
    switch (*field->desc)
    {
    case 'J':
    case 'D': {
        auto pp = zeroStackPopWGet<jlong>();
        *reinterpret_cast<jlong *>(oop->oopAccessField(zeroStackPopGet<OMElysiaOop *>(), field->offset)) = pp;
        break;
    }
    case 'L':
    case '[': {
        auto pp = zeroStackPopGet<OMElysiaOop *>();
        if (world->mainHeap.enablePtrCompress())
        {
            *reinterpret_cast<uint32_t *>(oop->oopAccessField(zeroStackPopGet<OMElysiaOop *>(), field->offset)) =
                world->mainHeap.compress(pp);
        }
        else
        {
            *reinterpret_cast<OMElysiaOop **>(oop->oopAccessField(zeroStackPopGet<OMElysiaOop *>(), field->offset)) =
                pp;
        }
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
