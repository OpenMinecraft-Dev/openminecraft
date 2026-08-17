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
#include "optimizations.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>

using namespace openminecraft::binary::hash;

namespace openminecraft::vm::elysia::executor
{
OMElysiaExecutorZero::OMElysiaExecutorZero(OMElysium *elysium) : elysium(elysium), logger("OMElysiaExecutorZero", this)
{
}
OMElysiaExecutorZero::~OMElysiaExecutorZero() = default;

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
HOT_FUNC
void OMElysiaExecutorZero::pushFrame(OMElysiaMethod *m, uint8_t *retAddr, bool needVtable, uint8_t **realpc)
{
    if (!m)
    {
        logger.dumpStacktrace();
        throw std::logic_error("function is null!");
    }

    if (m->intrinsic)
    {
        m->intrinsicRoutine(elysium, realpc, m->argSlots);
        return;
    }

    auto ll = m->argSlots;
    auto tc = thisThread.metadata;

    auto newlocal = reinterpret_cast<void *>(tc->zero.stackPointer - sizeof(OMElysiaJavaFrame));
    std::memmove(newlocal, reinterpret_cast<void *>(tc->zero.stackPointer), ll * sizeof(void *));

    zeroStackPop(ll * sizeof(void *));

    auto frame = reinterpret_cast<OMElysiaJavaFrame *>(zeroStackAlloc(sizeof(OMElysiaJavaFrame)));

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
    zeroStackAlloc(m->localLength * sizeof(void *));

    frame->method = m;
    frame->returnAddr = retAddr;
    frame->caller = tc->zero.frame;
    frame->objectRefs = nullptr;
    tc->zero.frame = frame;
    *realpc = m->code;
}

auto OMElysiaExecutorZero::currentKlassloader() -> OMElysiaKlassloader *
{
    return thisThread.metadata->zero.frame ? thisThread.metadata->zero.frame->method->klass->klassloader
                                           : elysium->klassLoader.get();
}

void OMElysiaExecutorZero::popFrame(uint8_t **realpc)
{
    auto &tczero = thisThread.metadata->zero;
    auto &frm = tczero.frame;

    tczero.stackPointer = reinterpret_cast<uintptr_t>(frm) + sizeof(OMElysiaJavaFrame);
    *realpc = tczero.frame->returnAddr;
    frm = frm->caller;
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

        tc->cleaner = []() -> void {
            auto tc = thisThread.metadata;
            mem::allocator::tracedFreeElysia(reinterpret_cast<void *>(tc->stackEnd));
            mem::allocator::tracedFreeElysia(tc->interface.internal);
        };
        tc->threadInited = true;
        tc->registerThread();

        if (!tc->special)
        {
            elysium->setupThreadObject();
        }
    }
}

HOT_FUNC
void OMElysiaExecutorZero::execute(OMElysiaMethod *m)
{
    auto tc = thisThread.metadata;
    auto oopManager = elysium->oopManager.get();

    auto cachedStackTop = tc->zero.stackPointer;

    uint8_t **superpc = tc->zero.pc;
    uint8_t *pc = tc->zero.pc ? *tc->zero.pc : nullptr;
    tc->zero.pc = &pc;
    OMElysiaJavaFrame *currentFrame = nullptr;
#define updateFrame currentFrame = tc->zero.frame;
    pushFrame(m, pc, false, &pc);

#define CURRENT_KLASS currentFrame->method->klass->toInstance()
    while (true)
    {
    loop_begin:
        updateFrame;

        // geopeila: the calling method's frame is popped, so we need to exit the interpreter loop
        if (tc->zero.stackPointer >= cachedStackTop)
        {
            tc->zero.pc = superpc;
            return;
        }

        if (tc->haveException)
        {
            auto frm = tc->zero.frame;
            if (frm->method->isNative())
            {
                tc->zero.pc = superpc;
                return;
            }
            for (int i = 0; i < frm->method->excTableLength; i++)
            {
                if (pc >= frm->method->excTable[i].begin && pc <= frm->method->excTable[i].end &&
                    oopManager->oopGetKlass(tc->currentException)->inherits(frm->method->excTable[i].type))
                {
                    pc = frm->method->excTable[i].handler;
                    zeroStackPush(tc->currentException);
                    tc->haveException = false;
                    tc->currentException = nullptr;
                    goto loop_begin;
                }
            }
            popFrame(&pc);
            continue;
        }

        if (tc->zero.frame->method->isNative())
        {
            execWithState(InsideVM, [&]() -> void { executeNativeLink(&pc, currentFrame); });
            continue;
        }

        thisThread.switchState(InsideJava);
        if (!pc)
        {
            throw std::logic_error("nullptr!");
        }

    exec:
        switch (*pc)
        {
        case op_nop:
            ++pc;
            goto exec;
        case op_aconst_null:
            ++pc;
            zeroStackPush<OMElysiaOop *>(nullptr);
            goto exec;
#define op_iconst(n)                                                                                                   \
    case op_iconst_i(n):                                                                                               \
        zeroStackPush<jint>(n);                                                                                        \
        ++pc;                                                                                                          \
        goto exec;

#define op_lconst(n)                                                                                                   \
    case op_lconst_l(n):                                                                                               \
        zeroStackPushW<jlong>(n);                                                                                      \
        ++pc;                                                                                                          \
        goto exec;

#define op_fconst(n)                                                                                                   \
    case op_fconst_f(n):                                                                                               \
        zeroStackPush<jfloat>(n);                                                                                      \
        ++pc;                                                                                                          \
        goto exec;

#define op_dconst(n)                                                                                                   \
    case op_dconst_d(n):                                                                                               \
        zeroStackPushW<jdouble>(n);                                                                                    \
        ++pc;                                                                                                          \
        goto exec;

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
            ++pc;
            zeroStackPush<jint>(static_cast<int8_t>(*pc));
            ++pc;
            goto exec;
        case op_sipush:
            zeroStackPush<jint>(zeroCodeFetchArgs16p0(pc));
            pc += 3;
            goto exec;
        case op_ldc:
            zeroStackPush(CURRENT_KLASS->constantPoolFetchNormal(pc[1], true));
            pc += 2;
            goto exec;
        case op_ldc_w:
            zeroStackPush(CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0(pc), true));
            pc += 3;
            goto exec;
        case op_ldc2_w:
            zeroStackPushW(CURRENT_KLASS->constantPoolFetchNormalW(zeroCodeFetchArgu16p0(pc)));
            pc += 3;
            goto exec;
#define op_iloadc(n)                                                                                                   \
    case op_iload_n(n):                                                                                                \
        zeroStackPush(zeroStackLoadLocal<jint>(n, currentFrame));                                                      \
        ++pc;                                                                                                          \
        goto exec;
            op_iloadc(0);
            op_iloadc(1);
            op_iloadc(2);
            op_iloadc(3);
        case op_iload:
            zeroStackPush(zeroStackLoadLocal<jint>(pc[1], currentFrame));
            pc += 2;
            goto exec;
#define op_lloadc(n)                                                                                                   \
    case op_lload_n(n):                                                                                                \
        zeroStackPushW(zeroStackLoadLocalW<jlong>(n, currentFrame));                                                   \
        ++pc;                                                                                                          \
        goto exec;
            op_lloadc(0);
            op_lloadc(1);
            op_lloadc(2);
            op_lloadc(3);
        case op_lload:
            zeroStackPushW(zeroStackLoadLocalW<jlong>(pc[1], currentFrame));
            pc += 2;
            goto exec;
#define op_floadc(n)                                                                                                   \
    case op_fload_n(n):                                                                                                \
        zeroStackPush(zeroStackLoadLocal<jfloat>(n, currentFrame));                                                    \
        ++pc;                                                                                                          \
        goto exec;
            op_floadc(0);
            op_floadc(1);
            op_floadc(2);
            op_floadc(3);
        case op_fload:
            zeroStackPush(zeroStackLoadLocal<jfloat>(pc[1], currentFrame));
            pc += 2;
            goto exec;
#define op_dloadc(n)                                                                                                   \
    case op_dload_n(n):                                                                                                \
        zeroStackPushW(zeroStackLoadLocalW<jdouble>(n, currentFrame));                                                 \
        ++pc;                                                                                                          \
        goto exec;
            op_dloadc(0);
            op_dloadc(1);
            op_dloadc(2);
            op_dloadc(3);
        case op_dload:
            zeroStackPushW(zeroStackLoadLocalW<jdouble>(pc[1], currentFrame));
            pc += 2;
            goto exec;
#define op_aloadc(n)                                                                                                   \
    case op_aload_n(n):                                                                                                \
        zeroStackPush(zeroStackLoadLocal<OMElysiaOop *>(n, currentFrame));                                             \
        ++pc;                                                                                                          \
        goto exec;
            op_aloadc(0);
            op_aloadc(1);
            op_aloadc(2);
            op_aloadc(3);
        case op_aload:
            zeroStackPush(zeroStackLoadLocal<OMElysiaOop *>(pc[1], currentFrame));
            pc += 2;
            goto exec;
#define xaload(op, type, store)                                                                                        \
    case op: {                                                                                                         \
        auto idx = zeroStackPopGet<jint>();                                                                            \
        auto obj = zeroStackPopGet<OMElysiaOop *>();                                                                   \
        store(oopManager->arrAccess<type>(obj)[idx]);                                                                  \
        ++pc;                                                                                                          \
        goto exec;                                                                                                     \
    }
            xaload(op_iaload, jint, zeroStackPush);
            xaload(op_laload, jlong, zeroStackPushW);
            xaload(op_faload, jfloat, zeroStackPush);
            xaload(op_daload, jdouble, zeroStackPushW);
        case op_aaload: {
            auto idx = zeroStackPopGet<jint>();
            auto obj = zeroStackPopGet<OMElysiaOop *>();
            zeroStackPush(oopManager->arrAccessPtr(obj, idx));
            ++pc;
            goto exec;
        }
            xaload(op_baload, jbyte, zeroStackPush);
            xaload(op_caload, jchar, zeroStackPush);
            xaload(op_saload, jshort, zeroStackPush);

#define op_istorec(n)                                                                                                  \
    case op_istore_n(n):                                                                                               \
        zeroStackSaveLocalPop<jint>(n, currentFrame);                                                                  \
        ++pc;                                                                                                          \
        goto exec;
            op_istorec(0);
            op_istorec(1);
            op_istorec(2);
            op_istorec(3);
        case op_istore:
            zeroStackSaveLocalPop<jint>(pc[1], currentFrame);
            pc += 2;
            goto exec;

#define op_lstorec(n)                                                                                                  \
    case op_lstore_n(n):                                                                                               \
        zeroStackSaveLocalPopW<jlong>(n, currentFrame);                                                                \
        ++pc;                                                                                                          \
        goto exec;
            op_lstorec(0);
            op_lstorec(1);
            op_lstorec(2);
            op_lstorec(3);
        case op_lstore:
            zeroStackSaveLocalPopW<jlong>(pc[1], currentFrame);
            pc += 2;
            goto exec;

#define op_fstorec(n)                                                                                                  \
    case op_fstore_n(n):                                                                                               \
        zeroStackSaveLocalPop<jfloat>(n, currentFrame);                                                                \
        ++pc;                                                                                                          \
        goto exec;
            op_fstorec(0);
            op_fstorec(1);
            op_fstorec(2);
            op_fstorec(3);
        case op_fstore:
            zeroStackSaveLocalPop<jfloat>(pc[1], currentFrame);
            pc += 2;
            goto exec;

#define op_dstorec(n)                                                                                                  \
    case op_dstore_n(n):                                                                                               \
        zeroStackSaveLocalPopW<jdouble>(n, currentFrame);                                                              \
        ++pc;                                                                                                          \
        goto exec;
            op_dstorec(0);
            op_dstorec(1);
            op_dstorec(2);
            op_dstorec(3);
        case op_dstore:
            zeroStackSaveLocalPopW<jdouble>(pc[1], currentFrame);
            pc += 2;
            goto exec;

#define op_astorec(n)                                                                                                  \
    case op_astore_n(n):                                                                                               \
        zeroStackSaveLocalPop<OMElysiaOop *>(n, currentFrame);                                                         \
        ++pc;                                                                                                          \
        goto exec;
            op_astorec(0);
            op_astorec(1);
            op_astorec(2);
            op_astorec(3);
        case op_astore:
            zeroStackSaveLocalPop<OMElysiaOop *>(pc[1], currentFrame);
            pc += 2;
            goto exec;

#define xastore(op, type, load)                                                                                        \
    case op: {                                                                                                         \
        auto value = load<type>();                                                                                     \
        auto index = zeroStackPopGet<jint>();                                                                          \
        auto arr = zeroStackPopGet<OMElysiaOop *>();                                                                   \
        oopManager->arrAccess<type>(arr)[index] = value;                                                               \
        ++pc;                                                                                                          \
        goto exec;                                                                                                     \
    }
            xastore(op_iastore, jint, zeroStackPopGet);
            xastore(op_lastore, jlong, zeroStackPopWGet);
            xastore(op_fastore, jfloat, zeroStackPopGet);
            xastore(op_dastore, jdouble, zeroStackPopWGet);
        case op_aastore: {
            auto value = zeroStackPopGet<OMElysiaOop *>();
            auto index = zeroStackPopGet<jint>();
            auto arr = zeroStackPopGet<OMElysiaOop *>();
            oopManager->arrAccessPtr(arr, index, value);
            ++pc;
            goto exec;
        }
            xastore(op_bastore, jboolean, zeroStackPopGet);
            xastore(op_castore, jchar, zeroStackPopGet);
            xastore(op_sastore, jshort, zeroStackPopGet);
        case op_pop:
            zeroStackPopGet<jint>();
            ++pc;
            goto exec;
        case op_pop2:
            zeroStackPopWGet<jlong>();
            ++pc;
            goto exec;
        case op_dup:
            zeroStackPush(zeroStackPeekGet<OMElysiaOop *>());
            ++pc;
            goto exec;
        case op_dup_x1: {
            auto value1 = zeroStackPopGet<OMElysiaOop *>();
            auto value2 = zeroStackPopGet<OMElysiaOop *>();

            zeroStackPush(value1);
            zeroStackPush(value2);
            zeroStackPush(value1);
            ++pc;
            goto exec;
        }
        case op_dup_x2: {
            auto value1 = zeroStackPopGet<OMElysiaOop *>();
            auto value2 = zeroStackPopGet<OMElysiaOop *>();
            auto value3 = zeroStackPopGet<OMElysiaOop *>();
            zeroStackPush(value1);
            zeroStackPush(value3);
            zeroStackPush(value2);
            zeroStackPush(value1);
            ++pc;
            goto exec;
        }
        case op_dup2: {
            auto value1 = zeroStackPopGet<OMElysiaOop *>();
            auto value2 = zeroStackPopGet<OMElysiaOop *>();

            zeroStackPush(value2);
            zeroStackPush(value1);
            zeroStackPush(value2);
            zeroStackPush(value1);
            ++pc;
            goto exec;
        }
        case op_dup2_x1: {
            auto value1 = zeroStackPopGet<OMElysiaOop *>();
            auto value2 = zeroStackPopGet<OMElysiaOop *>();
            auto value3 = zeroStackPopGet<OMElysiaOop *>();

            zeroStackPush(value2);
            zeroStackPush(value1);
            zeroStackPush(value3);
            zeroStackPush(value2);
            zeroStackPush(value1);
            ++pc;
            goto exec;
        }
        case op_dup2_x2: {
            auto value1 = zeroStackPopGet<OMElysiaOop *>();
            auto value2 = zeroStackPopGet<OMElysiaOop *>();
            auto value3 = zeroStackPopGet<OMElysiaOop *>();
            auto value4 = zeroStackPopGet<OMElysiaOop *>();

            zeroStackPush(value2);
            zeroStackPush(value1);
            zeroStackPush(value4);
            zeroStackPush(value3);
            zeroStackPush(value2);
            zeroStackPush(value1);
            ++pc;
            goto exec;
        }
        case op_swap: {
            auto value1 = zeroStackPopGet<OMElysiaOop *>();
            auto value2 = zeroStackPopGet<OMElysiaOop *>();
            zeroStackPush(value1);
            zeroStackPush(value2);
            ++pc;
            goto exec;
        }
#define op_calcinplace(op, fetch, fetchref, oprt)                                                                      \
    case op_##op: {                                                                                                    \
        auto value2 = fetch();                                                                                         \
        *fetchref() oprt value2;                                                                                       \
        ++pc;                                                                                                          \
        goto exec;                                                                                                     \
    }
#define op_calc(op, fetch, psh, oprt)                                                                                  \
    case op_##op: {                                                                                                    \
        auto value2 = fetch();                                                                                         \
        auto value1 = fetch();                                                                                         \
        psh(value1 oprt value2);                                                                                       \
        ++pc;                                                                                                          \
        goto exec;                                                                                                     \
    }
            op_calcinplace(iadd, zeroStackPopGet<jint>, zeroStackPeekGetRef<jint>, +=);
            op_calc(ladd, zeroStackPopWGet<jlong>, zeroStackPushW, +);
            op_calcinplace(fadd, zeroStackPopGet<jfloat>, zeroStackPeekGetRef<jfloat>, +=);
            op_calc(dadd, zeroStackPopWGet<jdouble>, zeroStackPushW, +);
            op_calcinplace(isub, zeroStackPopGet<jint>, zeroStackPeekGetRef<jint>, -=);
            op_calc(lsub, zeroStackPopWGet<jlong>, zeroStackPushW, -);
            op_calcinplace(fsub, zeroStackPopGet<jfloat>, zeroStackPeekGetRef<jfloat>, -=);
            op_calc(dsub, zeroStackPopWGet<jdouble>, zeroStackPushW, -);
            op_calcinplace(imul, zeroStackPopGet<jint>, zeroStackPeekGetRef<jint>, *=);
            op_calc(lmul, zeroStackPopWGet<jlong>, zeroStackPushW, *);
            op_calcinplace(fmul, zeroStackPopGet<jfloat>, zeroStackPeekGetRef<jfloat>, *=);
            op_calc(dmul, zeroStackPopWGet<jdouble>, zeroStackPushW, *);
        case op_idiv: {
            auto value2 = zeroStackPopGet<jint>();
            if (value2 == 0)
            {
                throw std::logic_error("divide by zero!");
            }
            *zeroStackPeekGetRef<jint>() /= value2;
            ++pc;
            goto exec;
        }
        case op_ldiv: {
            auto value2 = zeroStackPopWGet<jlong>();
            auto value1 = zeroStackPopWGet<jlong>();
            if (value2 == 0)
            {
                throw std::logic_error("divide by zero!");
            }
            zeroStackPushW(value1 / value2);
            ++pc;
            goto exec;
        }
            op_calcinplace(fdiv, zeroStackPopGet<jfloat>, zeroStackPeekGetRef<jfloat>, /=);
            op_calc(ddiv, zeroStackPopWGet<jdouble>, zeroStackPushW, /);

#define op_calcrem(op, fetch, psh)                                                                                     \
    case op_##op: {                                                                                                    \
        auto value2 = fetch();                                                                                         \
        auto value1 = fetch();                                                                                         \
        if (value2 == 0)                                                                                               \
        {                                                                                                              \
            throw std::logic_error("divide by zero!");                                                                 \
        }                                                                                                              \
        psh(value1 - (value1 / value2) * value2);                                                                      \
        ++pc;                                                                                                          \
        goto exec;                                                                                                     \
    }
            op_calcrem(irem, zeroStackPopGet<jint>, zeroStackPush);
            op_calcrem(lrem, zeroStackPopWGet<jlong>, zeroStackPushW);
        case op_frem: {
            auto value2 = zeroStackPopGet<jfloat>();
            auto value1 = zeroStackPopGet<jfloat>();
            zeroStackPush(std::fmod(value1, value2));
            ++pc;
            goto exec;
        }
        case op_drem: {
            auto value2 = zeroStackPopWGet<jdouble>();
            auto value1 = zeroStackPopWGet<jdouble>();
            zeroStackPushW(std::fmod(value1, value2));
            ++pc;
            goto exec;
        }

#define op_calcneg(op, fetch, psh)                                                                                     \
    case op_##op: {                                                                                                    \
        psh(-fetch());                                                                                                 \
        ++pc;                                                                                                          \
        goto exec;                                                                                                     \
    }
            op_calcneg(ineg, zeroStackPopGet<jint>, zeroStackPush);
            op_calcneg(lneg, zeroStackPopWGet<jlong>, zeroStackPushW);
            op_calcneg(fneg, zeroStackPopGet<jfloat>, zeroStackPush);
            op_calcneg(dneg, zeroStackPopWGet<jdouble>, zeroStackPushW);
        case op_ishl: {
            auto value2 = zeroStackPopGet<jint>();
            *zeroStackPeekGetRef<jint>() <<= (value2 & 0x1f);
            ++pc;
            goto exec;
        }
        case op_ishr: {
            auto value2 = zeroStackPopGet<jint>();
            *zeroStackPeekGetRef<jint>() >>= (value2 & 0x1f);
            ++pc;
            goto exec;
        }
        case op_iushr: {
            auto value2 = zeroStackPopGet<jint>();
            *zeroStackPeekGetRef<uint32_t>() >>= (value2 & 0x1f);
            ++pc;
            goto exec;
        }
        case op_lshl: {
            auto value2 = zeroStackPopGet<jint>();
            auto value1 = zeroStackPopWGet<jlong>();
            zeroStackPushW(value1 << (value2 & 0x3f));
            ++pc;
            goto exec;
        }
        case op_lshr: {
            auto value2 = zeroStackPopGet<jint>();
            auto value1 = zeroStackPopWGet<jlong>();
            zeroStackPushW(value1 >> (value2 & 0x3f));
            ++pc;
            goto exec;
        }
        case op_lushr: {
            auto value2 = zeroStackPopGet<jint>();
            auto value1 = zeroStackPopWGet<uint64_t>();
            zeroStackPushW(value1 >> (value2 & 0x3f));
            ++pc;
            goto exec;
        }
            op_calcinplace(iand, zeroStackPopGet<jint>, zeroStackPeekGetRef<jint>, &=);
            op_calc(land, zeroStackPopWGet<jlong>, zeroStackPushW, &);
            op_calcinplace(ior, zeroStackPopGet<jint>, zeroStackPeekGetRef<jint>, |=);
            op_calc(lor, zeroStackPopWGet<jlong>, zeroStackPushW, |);
            op_calcinplace(ixor, zeroStackPopGet<jint>, zeroStackPeekGetRef<jint>, ^=);
            op_calc(lxor, zeroStackPopWGet<jlong>, zeroStackPushW, ^);

        case op_iinc: {
            *zeroStackLoadLocalRef<jint>(pc[1], currentFrame) += static_cast<int8_t>(pc[2]);
            pc += 3;
            goto exec;
        }
#define op_conv(op, target, targettype, source, sourcetype)                                                            \
    case op_##op: {                                                                                                    \
        target(static_cast<targettype>(source<sourcetype>()));                                                         \
        ++pc;                                                                                                          \
        goto exec;                                                                                                     \
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
            op_conv(f2d, zeroStackPushW, jdouble, zeroStackPopGet, jfloat);
            op_conv(d2l, zeroStackPushW, jlong, zeroStackPopWGet, jdouble);
            op_conv(d2f, zeroStackPush, jfloat, zeroStackPopWGet, jdouble);
            op_conv(i2b, zeroStackPush, jbyte, zeroStackPopGet, jint);
            op_conv(i2c, zeroStackPush, jchar, zeroStackPopGet, jint);
            op_conv(i2s, zeroStackPush, jshort, zeroStackPopGet, jint);
#define op_ifcmp(cond, op)                                                                                             \
    case op_if##cond: {                                                                                                \
        if (zeroStackPopGet<jint>() op 0)                                                                              \
        {                                                                                                              \
            pc += zeroCodeFetchArgs16p0(pc);                                                                           \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            pc += 3;                                                                                                   \
        }                                                                                                              \
        goto exec;                                                                                                     \
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
            pc += zeroCodeFetchArgs16p0(pc);                                                                           \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            pc += 3;                                                                                                   \
        }                                                                                                              \
        goto exec;                                                                                                     \
    }
            op_ificmp(eq, ==);
            op_ificmp(ne, !=);
            op_ificmp(lt, <);
            op_ificmp(gt, >);
            op_ificmp(ge, >=);
            op_ificmp(le, <=);

        case op_lcmp: {
            auto value2 = zeroStackPopWGet<jlong>();
            auto value1 = zeroStackPopWGet<jlong>();
            if (value1 > value2)
            {
                zeroStackPush<jint>(1);
            }
            else if (value1 == value2)
            {
                zeroStackPush<jint>(0);
            }
            else
            {
                zeroStackPush<jint>(-1);
            }
            ++pc;
            goto exec;
        }
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
        ++pc;                                                                                                          \
        goto exec;                                                                                                     \
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
        ++pc;                                                                                                          \
        goto exec;                                                                                                     \
    }
            op_dcmp(g, 1);
            op_dcmp(l, -1);
        case op_if_acmpeq: {
            if (zeroStackPopGet<OMElysiaOop *>() == zeroStackPopGet<OMElysiaOop *>())
            {
                pc += zeroCodeFetchArgs16p0(pc);
            }
            else
            {
                pc += 3;
            }
            goto exec;
        }
        case op_if_acmpne: {
            if (zeroStackPopGet<OMElysiaOop *>() != zeroStackPopGet<OMElysiaOop *>())
            {
                pc += zeroCodeFetchArgs16p0(pc);
            }
            else
            {
                pc += 3;
            }
            goto exec;
        }
        case op_goto: {
            pc += zeroCodeFetchArgs16p0(pc);
            goto exec;
        }
        case op_tableswitch: {
            auto def = zeroCodeFetchArgs32Align(pc, 0);
            auto low = zeroCodeFetchArgs32Align(pc, 1);
            auto high = zeroCodeFetchArgs32Align(pc, 2);
            auto ll = zeroStackPopGet<jint>();
            if (ll > high || ll < low)
            {
                pc += def;
            }
            else
            {
                auto off = zeroCodeFetchArgs32Align(pc, 3 + (ll - low));
                pc += off;
            }
            goto exec;
        }
        case op_lookupswitch: {
            auto val = zeroStackPopGet<jint>();
            for (int i = 0; i < zeroCodeFetchArgs32Align(pc, 1); i++)
            {
                if (zeroCodeFetchArgs32Align(pc, 2 + i * 2) == val)
                {
                    pc += zeroCodeFetchArgs32Align(pc, 2 + i * 2 + 1);
                    goto lkend;
                }
            }
            pc += zeroCodeFetchArgs32Align(pc, 0);
        lkend:
            goto exec;
        }
        case op_return: {
            popFrame(&pc);
            updateFrame;
            continue;
        }
        case op_ireturn: {
            auto pp = zeroStackPopGet<jint>();
            popFrame(&pc);
            zeroStackPush(pp);
            updateFrame;
            continue;
        }
        case op_lreturn: {
            auto pp = zeroStackPopWGet<jlong>();
            popFrame(&pc);
            zeroStackPushW(pp);
            updateFrame;
            continue;
        }
        case op_freturn: {
            auto pp = zeroStackPopGet<jfloat>();
            popFrame(&pc);
            zeroStackPush(pp);
            updateFrame;
            continue;
        }
        case op_dreturn: {
            auto pp = zeroStackPopWGet<jdouble>();
            popFrame(&pc);
            zeroStackPushW(pp);
            updateFrame;
            continue;
        }
        case op_areturn: {
            auto pp = zeroStackPopGet<OMElysiaOop *>();
            popFrame(&pc);
            zeroStackPush(pp);
            updateFrame;
            continue;
        }
        case op_getstatic: {
            auto fld = CURRENT_KLASS->constantPoolFetchField(zeroCodeFetchArgu16p0(pc));
            zeroStackPushFromStatic(reinterpret_cast<OMElysiaField *>(fld), oopManager, elysium);
            pc += 3;
            goto exec;
        }
        case op_putfield: {
            auto fld = CURRENT_KLASS->constantPoolFetchField(zeroCodeFetchArgu16p0(pc));
            zeroStackPopToField(reinterpret_cast<OMElysiaField *>(fld), oopManager, elysium);
            pc += 3;
            goto exec;
        }
        case op_putstatic: {
            auto fld = CURRENT_KLASS->constantPoolFetchField(zeroCodeFetchArgu16p0(pc));
            zeroStackPopToStatic(reinterpret_cast<OMElysiaField *>(fld), oopManager, elysium);
            pc += 3;
            goto exec;
        }
        case op_getfield: {
            auto fld = CURRENT_KLASS->constantPoolFetchField(zeroCodeFetchArgu16p0(pc));
            zeroStackPushFromField(reinterpret_cast<OMElysiaField *>(fld), oopManager, elysium);
            pc += 3;
            goto exec;
        }
        case op_invokespecial:
        case op_invokestatic: {
            auto ff = CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0(pc));
            pushFrame(reinterpret_cast<OMElysiaMethod *>(ff), pc + 3, false, &pc);
            updateFrame;
            continue;
        }
        case op_invokevirtual: {
            auto ff = CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0(pc));
            pushFrame(reinterpret_cast<OMElysiaMethod *>(ff), pc + 3, true, &pc);
            updateFrame;
            continue;
        }
        case op_invokeinterface: {
            auto ff = CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0(pc));
            pushFrame(reinterpret_cast<OMElysiaMethod *>(ff), pc + 5, true, &pc);
            updateFrame;
            continue;
        }
        case op_invokedynamic: {
            auto m = reinterpret_cast<OMElysiaKlassDynamic *>(
                CURRENT_KLASS->constantPoolFetchDynamic(zeroCodeFetchArgu16p0(pc)));
            zeroStackPush(m->handle);
            pushFrame(m->target, pc + 5, false, &pc);
            updateFrame;
            continue;
        }
        case op_new: {
            auto c = CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0(pc));
            auto oop = oopManager->allocateOop(reinterpret_cast<OMElysiaKlass *>(c));
            zeroStackPush(oop);
            pc += 3;
            goto exec;
        }
        case op_newarray: {
            std::string kn;
            switch (pc[1])
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
                oopManager->allocateArr(CURRENT_KLASS->klassloader->findClass(kn)->toArray(), zeroStackPopGet<jint>());
            zeroStackPush(arr);
            pc += 2;
            goto exec;
        }
        case op_anewarray: {
            auto c = CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0(pc));
            auto klass = execWithState(InsideVM, [&]() -> OMElysiaKlass * {
                return CURRENT_KLASS->klassloader->fetchOrLoadClass(
                    buildArray(reinterpret_cast<OMElysiaKlass *>(c)->name), true);
            });
            auto arr = oopManager->allocateArr(klass->toArray(), zeroStackPopGet<jint>());
            zeroStackPush(arr);
            pc += 3;
            goto exec;
        }
        case op_arraylength: {
            zeroStackPush(oopManager->arrLength(zeroStackPopGet<OMElysiaOop *>()));
            ++pc;
            goto exec;
        }
        case op_athrow: {
            auto t = zeroStackPopGet<OMElysiaOop *>();
            elysium->throwException(t);
            continue;
        }
        case op_checkcast: {
            auto obj = zeroStackPopGet<OMElysiaOop *>();
            if (!obj)
            {
                zeroStackPush(obj);
            }
            else
            {
                auto c = reinterpret_cast<OMElysiaKlass *>(
                    CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0(pc)));
                if (oopManager->oopGetKlass(obj)->inherits(c))
                {
                    zeroStackPush(obj);
                }
                else
                {
                    throw std::logic_error(fmt::format("Java exception: ClassCastException, not implemented {} <=> {}",
                                                       c->name, oopManager->oopGetKlass(obj)->name));
                }
            }
            pc += 3;
            goto exec;
        }
        case op_instanceof: {
            auto obj = zeroStackPopGet<OMElysiaOop *>();
            if (!obj)
            {
                zeroStackPush<jint>(0);
            }
            else
            {
                auto c = reinterpret_cast<OMElysiaKlass *>(
                    CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0(pc)));
                zeroStackPush<jint>(oopManager->oopGetKlass(obj)->inherits(c) ? 1 : 0);
            }
            pc += 3;
            goto exec;
        }
        case op_ifnull: {
            if (!zeroStackPopGet<OMElysiaOop *>())
            {
                pc += zeroCodeFetchArgs16p0(pc);
            }
            else
            {
                pc += 3;
            }
            goto exec;
        }
        case op_ifnonnull: {
            if (zeroStackPopGet<OMElysiaOop *>())
            {
                pc += zeroCodeFetchArgs16p0(pc);
            }
            else
            {
                pc += 3;
            }
            goto exec;
        }
        case op_monitorexit: {
            auto obj = zeroStackPopGet<OMElysiaOop *>();
            elysium->monitorManager->mutexRelease(obj);
            ++pc;
            goto exec;
        }
        case op_monitorenter: {
            auto obj = zeroStackPopGet<OMElysiaOop *>();
            elysium->monitorManager->mutexFetch(obj);
            ++pc;
            goto exec;
        }
        case op_jsr: {
            zeroStackPush(pc + 3);
            pc += zeroCodeFetchArgs16p0(pc);
            goto exec;
        }
        case op_ret: {
            pc = zeroStackLoadLocal<uint8_t *>(pc[1], currentFrame);
            goto exec;
        }
        case op_goto_w: {
            pc += zeroCodeFetchArgs32p0(pc);
            goto exec;
        }
        case op_jsr_w: {
            zeroStackPush(pc + 5);
            pc += zeroCodeFetchArgs32p0(pc);
            goto exec;
        }
        case op_wide: {
            switch (pc[1])
            {
            case op_iload: {
                zeroStackPush(zeroStackLoadLocal<jint>(zeroCodeFetchArgu16p0(pc), currentFrame));
                pc += 4;
            }
            case op_istore: {
                zeroStackSaveLocalPop<jint>(zeroCodeFetchArgu16p0(pc), currentFrame);
                pc += 4;
                goto exec;
            }
            case op_lload: {
                zeroStackPushW(zeroStackLoadLocalW<jlong>(zeroCodeFetchArgu16p0(pc), currentFrame));
                pc += 4;
            }
            case op_lstore: {
                zeroStackSaveLocalPopW<jlong>(zeroCodeFetchArgu16p0(pc), currentFrame);
                pc += 4;
                goto exec;
            }
            case op_fload: {
                zeroStackPush(zeroStackLoadLocal<jfloat>(zeroCodeFetchArgu16p0(pc), currentFrame));
                pc += 4;
            }
            case op_fstore: {
                zeroStackSaveLocalPop<jfloat>(zeroCodeFetchArgu16p0(pc), currentFrame);
                pc += 4;
                goto exec;
            }
            case op_dload: {
                zeroStackPushW(zeroStackLoadLocalW<jdouble>(zeroCodeFetchArgu16p0(pc), currentFrame));
                pc += 4;
            }
            case op_dstore: {
                zeroStackSaveLocalPopW<jdouble>(zeroCodeFetchArgu16p0(pc), currentFrame);
                pc += 4;
                goto exec;
            }
            case op_ret: {
                pc = zeroStackLoadLocal<uint8_t *>(zeroCodeFetchArgu16p0(pc), currentFrame);
                goto exec;
            }
            case op_iinc: {
                *zeroStackLoadLocalRef<jint>(zeroCodeFetchArgu16p0(pc), currentFrame) = zeroCodeFetchArgs16p1(pc);
                pc += 6;
                goto exec;
            }
            }
            goto exec;
        }
        case op_multianewarray: {
            auto kls = CURRENT_KLASS->constantPoolFetchNormal(zeroCodeFetchArgu16p0(pc));
            std::vector<jint> sizes;
            sizes.resize(pc[3]);

            for (int i = pc[3] - 1; i >= 0; --i)
            {
                sizes[i] = zeroStackPopGet<jint>();
            }

            zeroStackPush(
                oopManager->allocateMultiArr(reinterpret_cast<OMElysiaArrayKlass *>(kls), pc[3], sizes.data()));

            pc += 4;
            goto exec;
        }
        default:
        unk: {
            logger.warn("unknown operand 0x{:02x}!", *pc);
            while (true)
            {
                continue;
            }
        }
        }
    }
}

auto OMElysiaExecutorZero::findRoutine(std::string klass, std::string name) -> OMElysiaIntrinsicRoutine
{
    if (klass == "java/lang/invoke/MethodHandle" && name == "linkToStatic")
    {
        return [](OMElysium *elysium, uint8_t **pc, int) -> void {
            auto mn = zeroStackPopGet<OMElysiaOop *>();
            auto remap = **pc == op_invokevirtual || **pc == op_invokeinterface;
            auto fieldoff = elysium->klassLoader->fetchOrLoadClass("java/lang/invoke/MemberName", true)
                                ->toInstance()
                                ->findField("<ptr>", "J")
                                ->offset;
            auto mthd = reinterpret_cast<OMElysiaMethod *>(
                *reinterpret_cast<jlong *>(elysium->oopManager->oopAccessField(mn, fieldoff)));
            elysium->executor->pushFrame(mthd, *pc + (**pc == op_invokeinterface ? 5 : 3), remap, pc);
        };
    }

    if (klass == "java/lang/invoke/MethodHandle" && (name == "invoke" || name == "invokeBasic"))
    {
        return [](OMElysium *elysium, uint8_t **pc, int a) -> void {
            auto hnd = *reinterpret_cast<OMElysiaOop **>(thisThread.metadata->zero.stackPointer + a * sizeof(void *));
            {
                auto ff = elysium->klassLoader->fetchOrLoadClass("java/lang/invoke/MethodHandle", true)
                              ->toInstance()
                              ->findField("form", "Ljava/lang/invoke/LambdaForm;")
                              ->offset;
                hnd = elysium->oopManager->oopAccessPointerField(hnd, ff);
            }
            {
                auto ff = elysium->klassLoader->fetchOrLoadClass("java/lang/invoke/LambdaForm", true)
                              ->toInstance()
                              ->findField("vmentry", "Ljava/lang/invoke/MemberName;")
                              ->offset;
                hnd = elysium->oopManager->oopAccessPointerField(hnd, ff);
            }
            auto remap = **pc == op_invokevirtual || **pc == op_invokeinterface;
            auto fieldoff = elysium->klassLoader->fetchOrLoadClass("java/lang/invoke/MemberName", true)
                                ->toInstance()
                                ->findField("<ptr>", "J")
                                ->offset;
            auto mthd = reinterpret_cast<OMElysiaMethod *>(
                *reinterpret_cast<jlong *>(elysium->oopManager->oopAccessField(hnd, fieldoff)));
            elysium->executor->pushFrame(mthd, *pc + (**pc == op_invokeinterface ? 5 : 3), remap, pc);
        };
    }
    return nullptr;
}
} // namespace openminecraft::vm::elysia::executor
