#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "fmt/base.h"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_field.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_meta.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <stdexcept>

using namespace openminecraft::binary::hash;

namespace openminecraft::vm::elysia::executor
{
OMElysiaExecutorZero::OMElysiaExecutorZero(OMElysium *elysium) : elysium(elysium), logger("OMElysiaExecutorZero", this)
{
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
void OMElysiaExecutorZero::pushFrame(OMElysiaMethod *m, uint8_t *retAddr, bool needVtable)
{
    if (!m)
    {
        throw std::logic_error("function is null!");
    }
    auto ll = argSlots(m->descriptor) + (m->isStatic() ? 0 : 1);
    auto tc = thisThread.metadata;

    auto newlocal = reinterpret_cast<void *>(tc->zero.stackPointer - sizeof(OMElysiaJavaFrame));
    std::memmove(newlocal, reinterpret_cast<void *>(tc->zero.stackPointer), ll * sizeof(void *));

    zeroStackPop(ll * sizeof(void *));

    auto frame = reinterpret_cast<OMElysiaJavaFrame *>(zeroStackAlloc(sizeof(OMElysiaJavaFrame)));
    zeroStackAlloc(ll * sizeof(void *));

    // function in vtable
    if ((!m->isStatic() && !m->isPrivate() && !m->isInit() && needVtable) || m->isAbstract())
    {
        auto oop = reinterpret_cast<OMElysiaOop **>(frame)[-1];
        if (!oop)
        {
            throw std::logic_error("nullptr!");
        }
        auto klass = elysium->oopManager->oopGetKlass(oop);

        if (klass->vtable && klass->vtableLength)
        {
            for (int i = 0; i < klass->vtableLength; i++)
            {
                if (klass->vtable[i]->isSame(m))
                {
                    m = klass->vtable[i];
                    goto nextStg;
                }
            }
        }

        throw std::logic_error("vtable not found!");
    }

nextStg:
    for (int i = ll; i < m->localLength; i++)
    {
        zeroStackPush<OMElysiaOop *>(nullptr);
    }

    frame->method = m;
    frame->returnAddr = retAddr;
    frame->caller = tc->zero.frame;
    frame->objectRefs = nullptr;
    tc->zero.frame = frame;
    tc->zero.pc = m->code;
}

void OMElysiaExecutorZero::popFrame()
{
    auto tc = thisThread.metadata;

    if (tc->zero.frame->method->isClinit())
    {
        tc->zero.frame->method->klass->klassMutex.unlock();
    }

    tc->zero.stackPointer = reinterpret_cast<uintptr_t>(tc->zero.frame) + sizeof(OMElysiaJavaFrame);
    tc->zero.pc = tc->zero.frame->returnAddr;
    tc->zero.frame = tc->zero.frame->caller;
}

void OMElysiaExecutorZero::threadInit()
{
    auto tc = thisThread.metadata;
    if (!tc->threadInited)
    {
        tc->stackEnd = reinterpret_cast<uintptr_t>(mem::allocator::tracedMallocElysia(1024 * 1024));
        tc->stackStart = tc->stackEnd + 1024 * 1024 - sizeof(void *);
        tc->zero.stackPointer = tc->stackStart;
        tc->interface.internal = reinterpret_cast<OMElysiaNativeInterface *>(
            mem::allocator::tracedCallocElysia(1, sizeof(OMElysiaNativeInterface)));
        tc->interface.internal->elysium = elysium;
        initBaseInterface(tc->interface);

        tc->cleaner = []() {
            auto tc = thisThread.metadata;
            mem::allocator::tracedFreeElysia(reinterpret_cast<void *>(tc->stackEnd));
            mem::allocator::tracedFreeElysia(tc->interface.internal);
        };
        tc->threadInited = true;
        tc->registerThread();

        execWithState(InsideVM, [&]() { elysium->setupThreadObject(); });
    }
}

void OMElysiaExecutorZero::execute(OMElysiaMethod *m)
{
    auto tc = thisThread.metadata;

    auto cachedStackTop = tc->zero.stackPointer;

    pushFrame(m, tc->zero.pc, false);

#define CURRENT_KLASS tc->zero.frame->method->klass->toInstance()

    while (true)
    {
        // geopeila: the calling method's frame is popped, so we need to exit the interpreter loop
        if (tc->zero.stackPointer >= cachedStackTop)
        {
            break;
        }

        if (tc->zero.frame->method->isNative())
        {
            tc->zero.pc = nullptr;
            execWithState(InsideVM, [&]() { executeNativeLink(); });
            continue;
        }

        thisThread.switchState(InsideJava);
        if (!tc->zero.pc)
        {
            throw std::logic_error("nullptr!");
        }
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
            zeroStackPush<jint>(zeroCodeFetchArgs16p0());
            tc->zero.pc += 3;
            break;
        case op_ldc: {
            zeroStackPush(CURRENT_KLASS->constantPoolFetchNormal(tc->zero.pc[1], true));
            tc->zero.pc += 2;
            break;
        }
        case op_ldc_w: {
            zeroStackPush(CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0(), true));
            tc->zero.pc += 3;
            break;
        }
        case op_ldc2_w: {
            zeroStackPushW(CURRENT_KLASS->constantPoolFetchNormalW(zeroCodeFetchArgu16p0()));
            tc->zero.pc += 3;
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
        case op_iload: {
            zeroStackPush(zeroStackLoadLocal<jint>(tc->zero.pc[1]));
            tc->zero.pc += 2;
            break;
        }
#define op_lloadc(n)                                                                                                   \
    case op_lload_n(n):                                                                                                \
        zeroStackPushW(zeroStackLoadLocal<jlong>(n));                                                                  \
        ++tc->zero.pc;                                                                                                 \
        break;
            op_lloadc(0);
            op_lloadc(1);
            op_lloadc(2);
        case op_lload: {
            zeroStackPushW(zeroStackLoadLocalW<jlong>(tc->zero.pc[1]));
            tc->zero.pc += 2;
            break;
        }
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
        case op_aload: {
            zeroStackPush(zeroStackLoadLocal<OMElysiaOop *>(tc->zero.pc[1]));
            tc->zero.pc += 2;
            break;
        }

        case op_caload: {
            auto idx = zeroStackPopGet<jint>();
            auto obj = zeroStackPopGet<OMElysiaOop *>();
            zeroStackPush(elysium->oopManager->arrAccess<jchar>(obj)[idx]);
            ++tc->zero.pc;
            break;
        }
        case op_iaload: {
            auto idx = zeroStackPopGet<jint>();
            auto obj = zeroStackPopGet<OMElysiaOop *>();
            zeroStackPush(elysium->oopManager->arrAccess<jint>(obj)[idx]);
            ++tc->zero.pc;
            break;
        }
        case op_aaload: {
            auto idx = zeroStackPopGet<jint>();
            auto obj = zeroStackPopGet<OMElysiaOop *>();
            zeroStackPush(elysium->oopManager->arrAccessPtr(obj, idx));
            ++tc->zero.pc;
            break;
        }

#define op_istorec(n)                                                                                                  \
    case op_istore_n(n):                                                                                               \
        zeroStackSaveLocalPop<jint>(n);                                                                                \
        ++tc->zero.pc;                                                                                                 \
        break;
            op_istorec(0);
            op_istorec(1);
            op_istorec(2);
            op_istorec(3);
        case op_istore: {
            zeroStackSaveLocalPop<jint>(tc->zero.pc[1]);
            tc->zero.pc += 2;
            break;
        }

#define op_astorec(n)                                                                                                  \
    case op_astore_n(n):                                                                                               \
        zeroStackSaveLocalPop<OMElysiaOop *>(n);                                                                       \
        ++tc->zero.pc;                                                                                                 \
        break;
            op_astorec(0);
            op_astorec(1);
            op_astorec(2);
            op_astorec(3);
        case op_astore: {
            zeroStackSaveLocalPop<OMElysiaOop *>(tc->zero.pc[1]);
            tc->zero.pc += 2;
            break;
        }

        case op_iastore: {
            auto value = zeroStackPopGet<jint>();
            auto index = zeroStackPopGet<jint>();
            auto arr = zeroStackPopGet<OMElysiaOop *>();
            elysium->oopManager->arrAccess<jint>(arr)[index] = value;
            ++tc->zero.pc;
            break;
        }
        case op_castore: {
            auto value = zeroStackPopGet<jchar>();
            auto index = zeroStackPopGet<jint>();
            auto arr = zeroStackPopGet<OMElysiaOop *>();
            elysium->oopManager->arrAccess<jchar>(arr)[index] = value;
            ++tc->zero.pc;
            break;
        }
        case op_aastore: {
            auto value = zeroStackPopGet<OMElysiaOop *>();
            auto index = zeroStackPopGet<jint>();
            auto arr = zeroStackPopGet<OMElysiaOop *>();
            elysium->oopManager->arrAccessPtr(arr, index, value);
            ++tc->zero.pc;
            break;
        }

        case op_pop:
            zeroStackPopGet<jint>();
            ++tc->zero.pc;
            break;
        case op_dup: {
            zeroStackPush(zeroStackPeekGet<OMElysiaOop *>());
            ++tc->zero.pc;
            break;
        }
        case op_dup_x1: {
            auto value1 = zeroStackPopGet<OMElysiaOop *>();
            auto value2 = zeroStackPopGet<OMElysiaOop *>();

            zeroStackPush(value1);
            zeroStackPush(value2);
            zeroStackPush(value1);
            ++tc->zero.pc;
            break;
        }
        case op_dup2: {
            auto value1 = zeroStackPopGet<OMElysiaOop *>();
            auto value2 = zeroStackPopGet<OMElysiaOop *>();

            zeroStackPush(value2);
            zeroStackPush(value1);
            zeroStackPush(value2);
            zeroStackPush(value1);
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
        case op_frem: {
            auto value2 = zeroStackPopGet<jfloat>();
            auto value1 = zeroStackPopGet<jfloat>();
            zeroStackPush(std::fmod(value1, value2));
            ++tc->zero.pc;
            break;
        }
        case op_drem: {
            auto value2 = zeroStackPopWGet<jdouble>();
            auto value1 = zeroStackPopWGet<jdouble>();
            zeroStackPushW(std::fmod(value1, value2));
            ++tc->zero.pc;
            break;
        }

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
        case op_lshl: {
            auto value2 = zeroStackPopGet<jint>();
            auto value1 = zeroStackPopWGet<jlong>();
            zeroStackPushW(value1 << value2);
            ++tc->zero.pc;
            break;
        }
            op_calc(ishr, zeroStackPopGet<jint>, zeroStackPush, >>);
        case op_lshr: {
            auto value2 = zeroStackPopGet<jint>();
            auto value1 = zeroStackPopWGet<jlong>();
            zeroStackPushW(value1 >> value2);
            ++tc->zero.pc;
            break;
        }
            op_calc(iushr, zeroStackPopGet<uint32_t>, zeroStackPush, >>);
        case op_lushr: {
            auto value2 = zeroStackPopGet<jint>();
            auto value1 = zeroStackPopWGet<uint64_t>();
            zeroStackPushW(value1 >> value2);
            ++tc->zero.pc;
            break;
        }
            op_calc(iand, zeroStackPopGet<jint>, zeroStackPush, &);
            op_calc(land, zeroStackPopWGet<jlong>, zeroStackPushW, &);
            op_calc(ior, zeroStackPopGet<jint>, zeroStackPush, |);
            op_calc(lor, zeroStackPopWGet<jlong>, zeroStackPushW, |);
            op_calc(ixor, zeroStackPopGet<jint>, zeroStackPush, ^);
            op_calc(lxor, zeroStackPopWGet<jlong>, zeroStackPushW, ^);

        case op_iinc: {
            auto slt = tc->zero.pc[1];
            jint data = zeroStackLoadLocal<jint>(slt);
            data += static_cast<int8_t>(tc->zero.pc[2]);
            zeroStackSaveLocal(slt, data);
            tc->zero.pc += 3;
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
        auto value2 = zeroStackPopGet<jint>();                                                                         \
        auto value1 = zeroStackPopGet<jint>();                                                                         \
        if (value1 op value2)                                                                                          \
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

#define op_fcmp(cond, n)                                                                                               \
    case op_fcmp##cond: {                                                                                              \
        auto value2 = zeroStackPopGet<jfloat>();                                                                       \
        auto value1 = zeroStackPopGet<jfloat>();                                                                       \
        if (std::isnan(value1) || std::isnan(value2))                                                                  \
        {                                                                                                              \
            zeroStackPush<jint>(n);                                                                                    \
        }                                                                                                              \
        else if (value1 > value2)                                                                                      \
        {                                                                                                              \
            zeroStackPush<jint>(1);                                                                                    \
        }                                                                                                              \
        else if (value1 < value2)                                                                                      \
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
            op_fcmp(g, 1);
            op_fcmp(l, -1);
#define op_dcmp(cond, n)                                                                                               \
    case op_dcmp##cond: {                                                                                              \
        auto value2 = zeroStackPopWGet<jdouble>();                                                                     \
        auto value1 = zeroStackPopWGet<jdouble>();                                                                     \
        if (std::isnan(value1) || std::isnan(value2))                                                                  \
        {                                                                                                              \
            zeroStackPush<jint>(n);                                                                                    \
        }                                                                                                              \
        else if (value1 > value2)                                                                                      \
        {                                                                                                              \
            zeroStackPush<jint>(1);                                                                                    \
        }                                                                                                              \
        else if (value1 < value2)                                                                                      \
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
            op_dcmp(g, 1);
            op_dcmp(l, -1);
        case op_if_acmpeq: {
            if (zeroStackPopGet<OMElysiaOop *>() == zeroStackPopGet<OMElysiaOop *>())
            {
                tc->zero.pc += zeroCodeFetchArgs16p0();
            }
            else
            {
                tc->zero.pc += 3;
            }
            break;
        }
        case op_if_acmpne: {
            if (zeroStackPopGet<OMElysiaOop *>() != zeroStackPopGet<OMElysiaOop *>())
            {
                tc->zero.pc += zeroCodeFetchArgs16p0();
            }
            else
            {
                tc->zero.pc += 3;
            }
            break;
        }
        case op_goto: {
            tc->zero.pc += zeroCodeFetchArgs16p0();
            break;
        }
        case op_lookupswitch: {
            auto val = zeroStackPopGet<jint>();
            for (int i = 0; i < zeroCodeFetchArgs32Align(1); i++)
            {
                if (zeroCodeFetchArgs32Align(2 + i * 2) == val)
                {
                    tc->zero.pc += zeroCodeFetchArgs32Align(2 + i * 2 + 1);
                    goto lkend;
                }
            }
            tc->zero.pc += zeroCodeFetchArgs32Align(0);
        lkend:
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
        case op_lreturn: {
            auto pp = zeroStackPopWGet<jlong>();
            popFrame();
            zeroStackPushW(pp);
            break;
        }
        case op_freturn: {
            auto pp = zeroStackPopGet<jfloat>();
            popFrame();
            zeroStackPush(pp);
            break;
        }
        case op_dreturn: {
            auto pp = zeroStackPopWGet<jdouble>();
            popFrame();
            zeroStackPushW(pp);
            break;
        }
        case op_areturn: {
            auto pp = zeroStackPopGet<OMElysiaOop *>();
            popFrame();
            zeroStackPush(pp);
            break;
        }
        case op_getstatic: {
            auto fld = CURRENT_KLASS->constantPoolFetchField(zeroCodeFetchArgu16p0());
            zeroStackPushFromStatic(reinterpret_cast<OMElysiaField *>(fld), elysium->oopManager.get(), elysium);
            tc->zero.pc += 3;
            break;
        }
        case op_putfield: {
            auto fld = CURRENT_KLASS->constantPoolFetchField(zeroCodeFetchArgu16p0());
            zeroStackPopToField(reinterpret_cast<OMElysiaField *>(fld), elysium->oopManager.get(), elysium);
            tc->zero.pc += 3;
            break;
        }
        case op_putstatic: {
            auto fld = CURRENT_KLASS->constantPoolFetchField(zeroCodeFetchArgu16p0());
            zeroStackPopToStatic(reinterpret_cast<OMElysiaField *>(fld), elysium->oopManager.get(), elysium);
            tc->zero.pc += 3;
            break;
        }
        case op_getfield: {
            auto fld = CURRENT_KLASS->constantPoolFetchField(zeroCodeFetchArgu16p0());
            zeroStackPushFromField(reinterpret_cast<OMElysiaField *>(fld), elysium->oopManager.get(), elysium);
            tc->zero.pc += 3;
            break;
        }
        case op_invokespecial:
        case op_invokestatic: {
            auto ff = CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0());
            pushFrame(reinterpret_cast<OMElysiaMethod *>(ff), tc->zero.pc + 3, false);
            break;
        }
        case op_invokevirtual: {
            auto ff = CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0());
            pushFrame(reinterpret_cast<OMElysiaMethod *>(ff), tc->zero.pc + 3, true);
            break;
        }
        case op_invokeinterface: {
            auto ff = CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0());
            pushFrame(reinterpret_cast<OMElysiaMethod *>(ff), tc->zero.pc + 5, true);
            break;
        }
        case op_new: {
            auto c = CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0());
            auto oop = elysium->oopManager->allocateOop(reinterpret_cast<OMElysiaKlass *>(c));
            zeroStackPush(oop);
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

            auto arr = elysium->oopManager->allocateArr(CURRENT_KLASS->klassloader->findClass(kn)->toArray(),
                                                        zeroStackPopGet<jint>());
            zeroStackPush(arr);
            tc->zero.pc += 2;
            break;
        }
        case op_anewarray: {
            auto c = CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0());
            OMElysiaKlass *klass;
            execWithState(InsideVM, [&]() {
                klass = CURRENT_KLASS->klassloader->fetchOrLoadClass(
                    buildArray(reinterpret_cast<OMElysiaKlass *>(c)->name));
            });
            auto arr = elysium->oopManager->allocateArr(klass->toArray(), zeroStackPopGet<jint>());
            zeroStackPush(arr);
            tc->zero.pc += 3;
            break;
        }
        case op_arraylength: {
            auto length = elysium->oopManager->arrLength(zeroStackPopGet<OMElysiaOop *>());
            zeroStackPush(length);
            ++tc->zero.pc;
            break;
        }
        case op_checkcast: {
            auto obj = zeroStackPopGet<OMElysiaOop *>();
            if (!obj)
            {
                zeroStackPush(obj);
            }
            else
            {
                auto c =
                    reinterpret_cast<OMElysiaKlass *>(CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0()));
                if (elysium->oopManager->oopGetKlass(obj)->inherits(c))
                {
                    zeroStackPush(obj);
                }
                else
                {
                    throw std::logic_error("Java exception: ClassCastException, not implemented");
                }
            }
            tc->zero.pc += 3;
            break;
        }
        case op_instanceof: {
            auto obj = zeroStackPopGet<OMElysiaOop *>();
            if (!obj)
            {
                zeroStackPush<jint>(0);
            }
            else
            {
                auto c =
                    reinterpret_cast<OMElysiaKlass *>(CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0()));
                zeroStackPush<jint>(elysium->oopManager->oopGetKlass(obj)->inherits(c) ? 1 : 0);
            }
            tc->zero.pc += 3;
            break;
        }
        case op_ifnull: {
            if (!zeroStackPopGet<OMElysiaOop *>())
            {
                tc->zero.pc += zeroCodeFetchArgs16p0();
            }
            else
            {
                tc->zero.pc += 3;
            }
            break;
        }
        case op_ifnonnull: {
            if (zeroStackPopGet<OMElysiaOop *>())
            {
                tc->zero.pc += zeroCodeFetchArgs16p0();
            }
            else
            {
                tc->zero.pc += 3;
            }
            break;
        }
        case op_monitorexit: {
            std::lock_guard guard(objectMonitorsMutex);
            auto obj = zeroStackPopGet<OMElysiaOop *>();
            objectMonitors[obj]->unlock();
            objectMonitors.erase(obj);
            ++tc->zero.pc;
            break;
        }
        case op_monitorenter: {
            std::lock_guard guard(objectMonitorsMutex);
            auto obj = zeroStackPopGet<OMElysiaOop *>();
            if (!objectMonitors.count(obj))
            {
                objectMonitors[obj] = std::make_shared<std::mutex>();
            }
            objectMonitors[obj]->lock();
            ++tc->zero.pc;
            break;
        }
        default:
        unk:
            throw std::logic_error(fmt::format("unknown operand! (operand 0x{:02x})", *tc->zero.pc));
        }
    }
}
} // namespace openminecraft::vm::elysia::executor
