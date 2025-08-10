#include "openminecraft/vm/pixeltower/v0/om_pixeltower_interpreter.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_field.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_frame.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_oop.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_threads.hpp"
#include "openminecraft/vm/pixeltower/v2/om_pixeltower_gc.hpp"
#include <any>
#include <cassert>
#include <functional>
#include <vector>

namespace openminecraft::vm::pixeltower::v0
{
OMInterpreter::OMInterpreter(OMPixelTowerHeap *heap, OMPixelTower *tower)
    : heap(heap), logger("OMInterpreter", this), tower(tower)
{
}
OMInterpreter::~OMInterpreter()
{
}

void OMInterpreter::call(OMMethod *met, uint8_t *retAddr)
{
    if (met->accessFlags & JVM_Acc_Native && !currentThread.currentFrame)
    {
        throw err::OMValidationError{err::Instructions, "native boot functions not supported!",
                                     fmt::format("{}.{}{}", met->klass->name, met->name, met->desc)};
    }

    if (!currentThread.currentFrame)
    {
        // geopeila: maybe the stack pointer need to be reset
        currentThread.stackPointer = currentThread.stack;
        auto frame = (OMFrame *)((uint8_t *)currentThread.stackPointer - sizeof(OMFrame));
        currentThread.stackPointer = (jbyte *)currentThread.stackPointer - sizeof(OMFrame) -
                                     met->maxLocals * sizeof(void *); // allocate whole frame + locals
        stackPush();
        frame->returnAddr = retAddr;
        frame->prev = nullptr;
        frame->method = met;
        currentThread.currentFrame = frame;
        currentThread.pc = met->code;
        for (int i = 0; i < met->maxLocals; i++)
        {
            localAccessMod<void *>(i, nullptr);
        }
        loop();
        return;
    }

    auto frame = currentThread.currentFrame;
    auto nextframe = (OMFrame *)((uint8_t *)currentThread.stackPointer - sizeof(OMFrame) + sizeof(void *) +
                                 met->args * sizeof(void *));
    nextframe->method = met;

    // TODO: optimization
    for (int i = 0; i < met->maxLocals; i++)
    {
        localAccessMod<void *>(i, nullptr, nextframe);
    }
    std::vector<void *> args(met->args);
    for (int i = met->args - 1; i >= 0; i--)
    {
        args[i] = stackTopAccess<void *>();
        stackPop();
    }

    nextframe->returnAddr = retAddr;
    nextframe->prev = frame;
    currentThread.stackPointer = (jbyte *)nextframe - met->maxLocals * sizeof(void *);
    currentThread.pc = met->code;
    currentThread.currentFrame = nextframe;
    stackPush();

    if (met->accessFlags & JVM_Acc_Native)
    {
        invokeNative(met, args);
        return;
    }

    for (int i = 0; i < args.size(); i++)
    {
        localAccessMod(i, args[i], nextframe);
    }

    loop();
}

void OMInterpreter::callDynamic(OMMethod *met, uint8_t *retAddr)
{
    auto frame = currentThread.currentFrame;
    auto nextframe = (OMFrame *)((uint8_t *)currentThread.stackPointer - sizeof(OMFrame) + sizeof(void *) +
                                 met->args * sizeof(void *));

    std::vector<void *> args(met->args);
    for (int i = met->args - 1; i >= 0; i--)
    {
        args[i] = stackTopAccess<void *>();
        stackPop();
    }

    auto cls = static_cast<OMOOPDesc *>(args[0])->klass;
    OMMethod *codetarget = (*cls->vtable)[fmt::format("{}{}", met->name, met->desc)];
    if (codetarget == nullptr)
    {
        throw err::OMValidationError{err::Instructions, fmt::format("vtable corruption found at klass {}", cls->name),
                                     methodName(frame->method)};
    }
    nextframe->method = codetarget;

    // context switching
    nextframe->returnAddr = retAddr;
    nextframe->prev = frame;
    currentThread.stackPointer = (jbyte *)nextframe - nextframe->method->maxLocals * sizeof(void *);
    currentThread.currentFrame = nextframe;
    stackPush();
    currentThread.pc = nextframe->method->code;

    if (codetarget->accessFlags & JVM_Acc_Native)
    {
        invokeNative(codetarget, args);
        return;
    }

    // clears local variable & pass arguments
    for (int i = 0; i < codetarget->maxLocals; i++)
    {
        localAccessMod<void *>(i, nullptr, nextframe);
    }
    for (int i = 0; i < args.size(); i++)
    {
        localAccessMod<void *>(i, args[i], nextframe);
    }

    loop();
}

void OMInterpreter::invokeNative(OMMethod *codetarget, std::vector<void *> &args)
{
    int p = 0;
    auto result = bytecode::descriptor::decodeSignature(codetarget->desc, &p);
    if (result.type == util::Err)
    {
        throw err::OMValidationError{
            err::Instructions, fmt::format("unknown function descriptor {}: {}", codetarget->desc, result.unwrap_err()),
            currentPosition()};
    }

    std::vector<std::any> data;

    auto itt = args.begin();
    if ((codetarget->accessFlags & JVM_Acc_Static) == 0)
    {
        data.push_back(*itt); // this pointer
        ++itt;
    }
    for (auto t : result.unwrap().first)
    {
        switch (hash_compile_time(t.c_str()))
        {
        case "boolean"_hash: {
            data.push_back((jboolean)(size_t)*itt);
            ++itt;
            break;
        }
        case "byte"_hash: {
            data.push_back((jbyte)(size_t)*itt);
            ++itt;
            break;
        }
        case "char"_hash: {
            data.push_back((jchar)(size_t)*itt);
            ++itt;
            break;
        }
        case "short"_hash: {
            data.push_back((jshort)(size_t)*itt);
            ++itt;
            break;
        }
        case "int"_hash: {
            data.push_back((jint)(size_t)*itt);
            ++itt;
            break;
        }
        case "float"_hash: {
            data.push_back(*reinterpret_cast<jfloat *>(&*itt));
            ++itt;
            break;
        }
        case "long"_hash: {
            if (sizeof(void *) == 8)
            {
                data.push_back(*reinterpret_cast<jlong *>(&*itt));
                ++itt;
            }
            else
            {
                auto low = (uint64_t)(size_t)*itt;
                ++itt;
                auto high = (uint64_t)(size_t)*itt;

                uint64_t raw = high << 32 | low;

                data.push_back(*reinterpret_cast<jlong *>(&raw));
            }
            ++itt;
            break;
        }
        case "double"_hash: {
            if (sizeof(void *) == 8)
            {
                data.push_back(*reinterpret_cast<jdouble *>(&*itt));
                ++itt;
            }
            else
            {
                auto low = (uint64_t)(size_t)*itt;
                ++itt;
                auto high = (uint64_t)(size_t)*itt;

                uint64_t raw = high << 32 | low;

                data.push_back(*reinterpret_cast<jdouble *>(&raw));
            }
            ++itt;
            break;
        }
        default: {
            data.push_back(*itt);
            ++itt;
            break;
        }
        }
    }

    auto funcp = *reinterpret_cast<std::function<std::any(std::any *)> **>(codetarget->code);
    if (funcp == nullFunction)
    {
        throw err::OMValidationError{err::Instructions, "unsatisfied link!", currentPosition()};
    }
    else
    {
        (*funcp)(data.data());
    }

    popLastFrame();
}

void OMInterpreter::popLastFrame()
{
    auto met = currentThread.currentFrame->method;
    currentThread.pc = (uint8_t *)currentThread.currentFrame->returnAddr;
    currentThread.stackPointer = (uint8_t *)currentThread.currentFrame + sizeof(OMFrame); // popped whole frame
    currentThread.currentFrame = currentThread.currentFrame->prev;
    stackPush();
}

jint OMInterpreter::execute()
{
    auto frame = currentThread.currentFrame;
operand:
    // gino: we need memory usage limit (etc >=60%), not the operand counter
    operands++;
    if (operands % 10000000 == 0)
    {
        logger.info("{} operands", operands);
        tower->gc->signUnreachable();
    }
    switch (currentThread.pc[0])
    {
    case op_iconst_i(-1):
    case op_iconst_i(0):
    case op_iconst_i(1):
    case op_iconst_i(2):
    case op_iconst_i(3):
    case op_iconst_i(4):
    case op_iconst_i(5): {
        stackPushAccess<jint>((jint)currentThread.pc[0] - (jint)op_iconst_i(0));
        currentThread.pc++;
        goto operand;
    }
    case op_lconst_l(0):
    case op_lconst_l(1): {
        stackPushAccessW<jlong>((jlong)currentThread.pc[0] - (jlong)op_lconst_l(0));
        currentThread.pc++;
        goto operand;
    }
    case op_bipush: {
        stackPushAccess<jint>((jint)currentThread.pc[1]);
        currentThread.pc += 2;
        goto operand;
    }
    case op_ldc: {
        auto n = static_cast<void **>(static_cast<void *>(frame->method->klass->constantPool + currentThread.pc[1]));
        assert(n != nullptr);
        stackPushAccess<void *>(*n);
        currentThread.pc += 2;
        goto operand;
    }
    case op_ldc2_w: {
        // compatible with jdouble
        auto n = static_cast<jlong *>(static_cast<void *>(frame->method->klass->constantPool +
                                                          binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1))));
        assert(n != nullptr);
        stackPushAccessW<jlong>(*n);
        currentThread.pc += 3;
        goto operand;
    }
    case op_aload: {
        stackPushAccess<void *>(localAccessValue<void *>(currentThread.pc[1]));
        currentThread.pc += 2;
        goto operand;
    }
    case op_iload_n(0):
    case op_iload_n(1):
    case op_iload_n(2):
    case op_iload_n(3): {
        stackPushAccess<jint>(localAccessValue<jint>(currentThread.pc[0] - op_iload_n(0)));
        currentThread.pc++;
        goto operand;
    }
    case op_lload_n(0):
    case op_lload_n(1):
    case op_lload_n(2):
    case op_lload_n(3): {
        stackPushAccessW<jlong>(localAccessValueW<jlong>(currentThread.pc[0] - op_lload_n(0)));
        currentThread.pc++;
        goto operand;
    }
    case op_aload_n(0):
    case op_aload_n(1):
    case op_aload_n(2):
    case op_aload_n(3): {
        stackPushAccess<void *>(localAccessValue<void *>(currentThread.pc[0] - op_aload_n(0)));
        currentThread.pc++;
        goto operand;
    }
    case op_baload: {
        auto idx = stackTopAccess<jint>();
        stackPop();
        auto arr = (OMOOPArrDesc *)stackTopAccess<void *>();
        stackPop();

        stackPushAccess<jint>(arr->array<jboolean>()[idx]);

        currentThread.pc++;
        goto operand;
    }
    case op_istore: {
        localAccessMod<jint>(currentThread.pc[1], stackTopAccess<jint>());
        stackPop();
        currentThread.pc += 2;
        goto operand;
    }
    case op_lstore: {
        localAccessModW<jlong>(currentThread.pc[1], stackTopAccessW<jlong>());
        stackPopW();
        currentThread.pc += 2;
        goto operand;
    }
    case op_astore: {
        localAccessMod<void *>(currentThread.pc[1], stackTopAccess<void *>());
        stackPop();
        currentThread.pc += 2;
        goto operand;
    }
    case op_istore_n(0):
    case op_istore_n(1):
    case op_istore_n(2):
    case op_istore_n(3): {
        localAccessMod<jint>(currentThread.pc[0] - op_istore_n(0), stackTopAccess<jint>());
        stackPop();
        currentThread.pc++;
        goto operand;
    }
    case op_astore_n(0):
    case op_astore_n(1):
    case op_astore_n(2):
    case op_astore_n(3): {
        localAccessMod<void *>(currentThread.pc[0] - op_astore_n(0), stackTopAccess<void *>());
        stackPop();
        currentThread.pc++;
        goto operand;
    }
    case op_bastore: {
        auto value = (jboolean)stackTopAccess<jint>();
        stackPop();
        auto idx = stackTopAccess<jint>();
        stackPop();
        auto arr = (OMOOPArrDesc *)stackTopAccess<void *>();
        stackPop();

        arr->array<jboolean>()[idx] = value;

        currentThread.pc++;
        goto operand;
    }
    case op_pop: {
        stackPop();
        currentThread.pc++;
        goto operand;
    }
    case op_pop2: {
        stackPopW();
        currentThread.pc++;
        goto operand;
    }
    case op_dup: {
        stackPushAccess<void *>(stackTopAccess<void *>());
        currentThread.pc++;
        goto operand;
    }
    case op_iadd: {
        auto i1 = stackTopAccess<jint>();
        stackPop();
        auto i2 = stackTopAccess<jint>();
        stackPop();
        stackPushAccess<jint>(i1 + i2);
        currentThread.pc++;
        goto operand;
    }
    case op_ladd: {
        auto item2 = stackTopAccessW<jlong>();
        stackPopW();
        auto item = stackTopAccessW<jlong>();
        stackPopW();
        stackPushAccessW<jlong>(item2 + item);
        currentThread.pc++;
        goto operand;
    }
    case op_iinc: {
        localAccessMod<jint>(currentThread.pc[1], localAccessValue<jint>(currentThread.pc[1]) + currentThread.pc[2]);
        currentThread.pc += 3;
        goto operand;
    }
    case op_i2l: {
        auto v = stackTopAccess<jint>();
        stackPop();
        stackPushAccessW<jlong>(v);
        currentThread.pc++;
        goto operand;
    }
    case op_i2b: {
        auto v = stackTopAccess<jint>();
        stackPop();
        stackPushAccess<jint>((jint)(jbyte)v);
        currentThread.pc++;
        goto operand;
    }
    case op_i2s: {
        auto v = stackTopAccess<jint>();
        stackPop();
        stackPushAccess<jint>((jint)(jshort)v);
        currentThread.pc++;
        goto operand;
    }
    case op_lcmp: {
        auto item2 = stackTopAccessW<jlong>();
        stackPopW();
        auto item = stackTopAccessW<jlong>();
        stackPopW();
        if (item > item2)
        {
            stackPushAccess<jint>(1);
        }
        else if (item == item2)
        {
            stackPushAccess<jint>(0);
        }
        else
        {
            stackPushAccess<jint>(-1);
        }
        currentThread.pc++;
        goto operand;
    }
    case op_ifne: {
        auto i = stackTopAccess<jint>();
        stackPop();
        if (i != 0)
        {
            currentThread.pc += binary::be16SignedToNative(currentThread.pc[1], currentThread.pc[2]);
        }
        else
        {
            currentThread.pc += 3;
        }
        goto operand;
    }
    case op_ifge: {
        auto i = stackTopAccess<jint>();
        stackPop();
        if (i >= 0)
        {
            currentThread.pc += binary::be16SignedToNative(currentThread.pc[1], currentThread.pc[2]);
        }
        else
        {
            currentThread.pc += 3;
        }
        goto operand;
    }
    case op_if_acmpne: {
        auto item = stackTopAccess<void *>();
        stackPop();
        auto item2 = stackTopAccess<void *>();
        stackPop();
        if (item != item2)
        {
            currentThread.pc += binary::be16SignedToNative(currentThread.pc[1], currentThread.pc[2]);
        }
        else
        {
            currentThread.pc += 3;
        }

        goto operand;
    }
    case op_goto: {
        currentThread.pc += binary::be16SignedToNative(currentThread.pc[1], currentThread.pc[2]);
        goto operand;
    }
    case op_ireturn: {
        auto ret = stackTopAccess<jint>();
        popLastFrame();
        stackPushAccess<jint>(ret);
        return EXEC_RETURN;
    }
    case op_lreturn: {
        auto ret = stackTopAccessW<jlong>();
        popLastFrame();
        stackPushAccessW<jlong>(ret);
        return EXEC_RETURN;
    }
    case op_return: {
        popLastFrame();
        return EXEC_RETURN;
    }
    case op_getstatic: {
        auto id = binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1));
        auto n = *static_cast<OMField **>(static_cast<void *>(frame->method->klass->constantPool + id));
        if (n == nullptr)
        {
            n = tower->loader->lazyFieldInit(currentThread.currentFrame->method->klass, id);
        }
        assert(n != nullptr);
        fetchFieldStatic(n);
        currentThread.pc += 3;
        goto operand;
    }
    case op_putstatic: {
        auto id = binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1));
        auto n = *static_cast<OMField **>(static_cast<void *>(frame->method->klass->constantPool + id));
        if (n == nullptr)
        {
            n = tower->loader->lazyFieldInit(currentThread.currentFrame->method->klass, id);
        }
        assert(n != nullptr);
        accessFieldStatic(n);
        currentThread.pc += 3;

        goto operand;
    }
    case op_putfield: {
        auto id = binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1));
        auto n = *static_cast<OMField **>(static_cast<void *>(frame->method->klass->constantPool + id));
        if (n == nullptr)
        {
            n = tower->loader->lazyFieldInit(currentThread.currentFrame->method->klass, id);
        }
        assert(n != nullptr);
        accessField(n);
        currentThread.pc += 3;

        goto operand;
    }
    case op_invokevirtual: {
        auto id = binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1));
        auto n = *static_cast<OMMethod **>(static_cast<void *>(frame->method->klass->constantPool + id));
        if (n == nullptr)
        {
            n = tower->loader->lazyMethodInit(currentThread.currentFrame->method->klass, id);
        }
        assert(n != nullptr);
        callDynamic(n, currentThread.pc);
        currentThread.pc += 3;

        goto operand;
    }
    case op_invokespecial: {
        auto id = binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1));
        auto n = *static_cast<OMMethod **>(static_cast<void *>(frame->method->klass->constantPool + id));
        if (n == nullptr)
        {
            n = tower->loader->lazyMethodInit(currentThread.currentFrame->method->klass, id);
        }
        assert(n != nullptr);
        call(n, currentThread.pc);
        currentThread.pc += 3;

        goto operand;
    }
    case op_invokestatic: {
        auto id = binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1));
        auto n = *static_cast<OMMethod **>(static_cast<void *>(frame->method->klass->constantPool + id));
        if (n == nullptr)
        {
            n = tower->loader->lazyMethodInit(currentThread.currentFrame->method->klass, id);
        }
        assert(n != nullptr);
        call(n, currentThread.pc);
        currentThread.pc += 3;

        goto operand;
    }
    case op_new: {
        auto id = binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1));
        auto n = *static_cast<OMKlass **>(static_cast<void *>(frame->method->klass->constantPool + id));
        if (n == nullptr)
        {
            n = tower->loader->lazyClassInit(currentThread.currentFrame->method->klass, id);
        }
        assert(n != nullptr);
        stackPushAccess<void *>(n->allocateInstance());
        currentThread.pc += 3;
        goto operand;
    }
    case op_newarray: {
        OMKlass *arrkl;
#define mapAndLoad(id, name)                                                                                           \
    case id:                                                                                                           \
        tower->loader->loadClass(name);                                                                                \
        arrkl = tower->loader->fetchClass(name);                                                                       \
        break;

        switch (currentThread.pc[1])
        {
            mapAndLoad(4, "[Z");
            mapAndLoad(5, "[C");
            mapAndLoad(6, "[F");
            mapAndLoad(7, "[D");
            mapAndLoad(8, "[B");
            mapAndLoad(9, "[S");
            mapAndLoad(10, "[I");
            mapAndLoad(11, "[J");
        }

        auto r = arrkl->allocateArray(stackTopAccess<jint>());
        stackPop();
        stackPushAccess<void *>(r);

        currentThread.pc += 2;
        goto operand;
    }
    default: {
        logger.error("We are hitting the Mazarine End!");
        logger.error("unknown operand at {} ({:#04x})", fmt::ptr(currentThread.pc), (int)*currentThread.pc);
        logger.error("thread {}", fmt::ptr(&currentThread.id));
        logger.error("pc pointed at {} ({} at {})", fmt::ptr(currentThread.pc), currentPosition(),
                     fmt::ptr(currentThread.currentFrame->method->code));

        debugger.debugStack();

        break;
    }
    }
    return EXEC_FAIL;
}
} // namespace openminecraft::vm::pixeltower::v0
