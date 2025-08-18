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
#include <cmath>
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
        // geopelia: maybe the stack pointer need to be reset
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
    operands++;
    // gino: we need memory usage limit (etc >=60%), not the operand counter (solved)
    // gino: implemented!
    if (heap->usage() >= 0.6)
    {
        logger.info("main heap usage: {:.2f}% used ({:.2f}% of total)", heap->usage() * 100, heap->totalUsage() * 100);
        logger.info("metaspace heap usage: {:.2f}% used ({:.2f}% of total)", tower->metaspace->usage() * 100,
                    tower->metaspace->usage() * 100);
        tower->gc->signUnreachable();
    }

    switch (currentThread.pc[0])
    {
    case op_nop: {
        currentThread.pc++;
        goto operand;
    }
    case op_aconst_null: {
        stackPushAccess<void *>(nullptr);
        currentThread.pc++;
        goto operand;
    }
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
    case op_fconst_f(0.0f):
    case op_fconst_f(1.0f): {
    case op_fconst_f(2.0f):
        stackPushAccess<jfloat>((jfloat)(currentThread.pc[0] - op_fconst_f(0.0f)));
        currentThread.pc++;
        goto operand;
    }
    case op_dconst_d(0.0):
    case op_dconst_d(1.0): {
        stackPushAccessW<jdouble>((jdouble)(currentThread.pc[0] - op_dconst_d(0.0)));
        currentThread.pc++;
        goto operand;
    }
    case op_bipush: {
        stackPushAccess<jint>((jint)currentThread.pc[1]);
        currentThread.pc += 2;
        goto operand;
    }
    case op_sipush: {
        stackPushAccess<jint>(binary::be16SignedToNative(currentThread.pc[1], currentThread.pc[2]));
        currentThread.pc += 3;
        goto operand;
    }
    case op_ldc: {
        auto n = static_cast<void **>(static_cast<void *>(frame->method->klass->constantPool + currentThread.pc[1]));
        if (!n)
        {
            throwTypeCheckError("there is no constant here!");
        }
        stackPushAccess<void *>(*n);
        currentThread.pc += 2;
        goto operand;
    }
    case op_ldc_w: {
        auto n = static_cast<void **>(static_cast<void *>(frame->method->klass->constantPool +
                                                          binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1))));
        if (!n)
        {
            throwTypeCheckError("there is no constant here!");
        }
        stackPushAccess<void *>(*n);
        currentThread.pc += 3;
        goto operand;
    }
    case op_ldc2_w: {
        // gino: compatible with jdouble
        auto n = static_cast<jlong *>(static_cast<void *>(frame->method->klass->constantPool +
                                                          binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1))));
        if (!n)
        {
            throwTypeCheckError("there is no constant here!");
        }
        stackPushAccessW<jlong>(*n);
        currentThread.pc += 3;
        goto operand;
    }
    case op_iload: {
        stackPushAccess<jint>(localAccessValue<jint>(currentThread.pc[1]));
        currentThread.pc += 2;
        goto operand;
    }
    case op_lload: {
        stackPushAccessW<jlong>(localAccessValueW<jlong>(currentThread.pc[1]));
        currentThread.pc += 2;
        goto operand;
    }
    case op_fload: {
        stackPushAccess<jfloat>(localAccessValue<jfloat>(currentThread.pc[1]));
        currentThread.pc += 2;
        goto operand;
    }
    case op_dload: {
        stackPushAccessW<jdouble>(localAccessValueW<jdouble>(currentThread.pc[1]));
        currentThread.pc += 2;
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
    case op_fload_n(0):
    case op_fload_n(1):
    case op_fload_n(2):
    case op_fload_n(3): {
        stackPushAccess<jfloat>(localAccessValue<jfloat>(currentThread.pc[0] - op_fload_n(0)));
        currentThread.pc++;
        goto operand;
    }
    case op_dload_n(0):
    case op_dload_n(1):
    case op_dload_n(2):
    case op_dload_n(3): {
        stackPushAccessW<jdouble>(localAccessValueW<jdouble>(currentThread.pc[0] - op_dload_n(0)));
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
    case op_iaload: {
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isIntArr())
        {
            throwTypeCheckError("it's not an int array!");
        }

        stackPushAccess<jint>(arr->array<jint>()[idx]);

        currentThread.pc++;
        goto operand;
    }
    case op_laload: {
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isLongArr())
        {
            throwTypeCheckError("it's not an long array!");
        }

        stackPushAccessW<jlong>(arr->array<jlong>()[idx]);

        currentThread.pc++;
        goto operand;
    }
    case op_faload: {
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isFloatArr())
        {
            throwTypeCheckError("it's not an float array!");
        }

        stackPushAccess<jfloat>(arr->array<jfloat>()[idx]);

        currentThread.pc++;
        goto operand;
    }
    case op_daload: {
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isDoubleArr())
        {
            throwTypeCheckError("it's not an double array!");
        }

        stackPushAccessW<jdouble>(arr->array<jdouble>()[idx]);

        currentThread.pc++;
        goto operand;
    }
    case op_aaload: {
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isObjArr())
        {
            throwTypeCheckError("it's not an reference array!");
        }

        if (heap->ptrCompEnabled())
        {
            stackPushAccess<void *>(heap->decompressPtr(arr->array<uint32_t>()[idx]));
        }
        else
        {
            stackPushAccess<void *>(arr->array<void *>()[idx]);
        }

        currentThread.pc++;
        goto operand;
    }
    case op_baload: {
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isByteArr() && !arr->klass->isBooleanArr())
        {
            throwTypeCheckError("it's not an byte or boolean array!");
        }

        stackPushAccess<jint>(arr->array<jboolean>()[idx]);

        currentThread.pc++;
        goto operand;
    }
    case op_caload: {
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isCharArr())
        {
            throwTypeCheckError("it's not an char array!");
        }

        stackPushAccess<jint>(arr->array<jchar>()[idx]);

        currentThread.pc++;
        goto operand;
    }
    case op_saload: {
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isShortArr())
        {
            throwTypeCheckError("it's not an short array!");
        }

        stackPushAccess<jint>(arr->array<jshort>()[idx]);

        currentThread.pc++;
        goto operand;
    }
    case op_istore: {
        localAccessMod<jint>(currentThread.pc[1], stackTopAccess<jint>(true));
        currentThread.pc += 2;
        goto operand;
    }
    case op_lstore: {
        localAccessModW<jlong>(currentThread.pc[1], stackTopAccessW<jlong>(true));
        currentThread.pc += 2;
        goto operand;
    }
    case op_fstore: {
        localAccessMod<jfloat>(currentThread.pc[1], stackTopAccess<jfloat>(true));
        currentThread.pc += 2;
        goto operand;
    }
    case op_dstore: {
        localAccessModW<jdouble>(currentThread.pc[1], stackTopAccessW<jdouble>(true));
        currentThread.pc += 2;
        goto operand;
    }
    case op_astore: {
        localAccessMod<void *>(currentThread.pc[1], stackTopAccess<void *>(true));
        currentThread.pc += 2;
        goto operand;
    }
    case op_istore_n(0):
    case op_istore_n(1):
    case op_istore_n(2):
    case op_istore_n(3): {
        localAccessMod<jint>(currentThread.pc[0] - op_istore_n(0), stackTopAccess<jint>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_lstore_n(0):
    case op_lstore_n(1):
    case op_lstore_n(2):
    case op_lstore_n(3): {
        localAccessModW<jlong>(currentThread.pc[0] - op_lstore_n(0), stackTopAccessW<jlong>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_fstore_n(0):
    case op_fstore_n(1):
    case op_fstore_n(2):
    case op_fstore_n(3): {
        localAccessMod<jfloat>(currentThread.pc[0] - op_fstore_n(0), stackTopAccess<jfloat>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_dstore_n(0):
    case op_dstore_n(1):
    case op_dstore_n(2):
    case op_dstore_n(3): {
        localAccessModW<jdouble>(currentThread.pc[0] - op_dstore_n(0), stackTopAccessW<jdouble>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_astore_n(0):
    case op_astore_n(1):
    case op_astore_n(2):
    case op_astore_n(3): {
        localAccessMod<void *>(currentThread.pc[0] - op_astore_n(0), stackTopAccess<void *>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_iastore: {
        auto value = stackTopAccess<jint>(true);
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isIntArr())
        {
            throwTypeCheckError("it's not an int array!");
        }

        arr->array<jint>()[idx] = value;

        currentThread.pc++;
        goto operand;
    }
    case op_lastore: {
        auto value = stackTopAccessW<jlong>(true);
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isLongArr())
        {
            throwTypeCheckError("it's not an long array!");
        }

        arr->array<jlong>()[idx] = value;

        currentThread.pc++;
        goto operand;
    }
    case op_fastore: {
        auto value = stackTopAccess<jfloat>(true);
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isFloatArr())
        {
            throwTypeCheckError("it's not an float array!");
        }

        arr->array<jfloat>()[idx] = value;

        currentThread.pc++;
        goto operand;
    }
    case op_dastore: {
        auto value = stackTopAccessW<jdouble>(true);
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isDoubleArr())
        {
            throwTypeCheckError("it's not an double array!");
        }

        arr->array<jdouble>()[idx] = value;

        currentThread.pc++;
        goto operand;
    }
    case op_bastore: {
        auto value = (jboolean)stackTopAccess<jint>(true);
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isByteArr() && !arr->klass->isBooleanArr())
        {
            throwTypeCheckError("it's not an byte or boolean array!");
        }

        arr->array<jboolean>()[idx] = value;

        currentThread.pc++;
        goto operand;
    }
    case op_aastore: {
        auto value = stackTopAccess<void *>(true);
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isObjArr())
        {
            throwTypeCheckError("it's not an reference array!");
        }

        if (heap->ptrCompEnabled())
        {
            arr->array<uint32_t>()[idx] = heap->compressPtr(value);
        }
        else
        {
            arr->array<void *>()[idx] = value;
        }

        currentThread.pc++;
        goto operand;
    }
    case op_castore: {
        auto value = stackTopAccess<jint>(true);
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isCharArr())
        {
            throwTypeCheckError("it's not an char array!");
        }

        arr->array<jchar>()[idx] = value;

        currentThread.pc++;
        goto operand;
    }
    case op_sastore: {
        auto value = stackTopAccess<jint>(true);
        auto idx = stackTopAccess<jint>(true);
        auto arr = stackTopAccess<OMOOPArrDesc *>(true);

        if (!arr->klass->isShortArr())
        {
            throwTypeCheckError("it's not an short array!");
        }

        arr->array<jshort>()[idx] = value;

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
    case op_dup_x1: {
        auto value1 = stackTopAccess<void *>(true);
        auto value2 = stackTopAccess<void *>(true);
        stackPushAccess<void *>(value1);
        stackPushAccess<void *>(value2);
        stackPushAccess<void *>(value1);
        currentThread.pc++;
        goto operand;
    }
    case op_dup_x2: {
        auto value1 = stackTopAccess<void *>(true);
        auto value2 = stackTopAccess<void *>(true);
        auto value3 = stackTopAccess<void *>(true);
        stackPushAccess<void *>(value1);
        stackPushAccess<void *>(value3);
        stackPushAccess<void *>(value2);
        stackPushAccess<void *>(value1);
        currentThread.pc++;
        goto operand;
    }
    case op_dup2: {
        auto value1 = stackTopAccess<void *>(true);
        auto value2 = stackTopAccess<void *>();
        stackPushAccess<void *>(value1);
        stackPushAccess<void *>(value2);
        stackPushAccess<void *>(value1);
        currentThread.pc++;
        goto operand;
    }
    case op_dup2_x1: {
        auto value1 = stackTopAccess<void *>(true);
        auto value2 = stackTopAccess<void *>(true);
        auto value3 = stackTopAccess<void *>(true);
        stackPushAccess<void *>(value2);
        stackPushAccess<void *>(value1);
        stackPushAccess<void *>(value3);
        stackPushAccess<void *>(value2);
        stackPushAccess<void *>(value1);
        currentThread.pc++;
        goto operand;
    }
    case op_dup2_x2: {
        auto value1 = stackTopAccess<void *>(true);
        auto value2 = stackTopAccess<void *>(true);
        auto value3 = stackTopAccess<void *>(true);
        auto value4 = stackTopAccess<void *>(true);
        stackPushAccess<void *>(value2);
        stackPushAccess<void *>(value1);
        stackPushAccess<void *>(value4);
        stackPushAccess<void *>(value3);
        stackPushAccess<void *>(value2);
        stackPushAccess<void *>(value1);
        currentThread.pc++;
        goto operand;
    }
    case op_swap: {
        auto value1 = stackTopAccess<void *>(true);
        auto value2 = stackTopAccess<void *>(true);
        stackPushAccess<void *>(value1);
        stackPushAccess<void *>(value2);
        currentThread.pc++;
        goto operand;
    }
    case op_iadd: {
        auto i1 = stackTopAccess<jint>(true);
        auto i2 = stackTopAccess<jint>(true);
        stackPushAccess<jint>(i1 + i2);
        currentThread.pc++;
        goto operand;
    }
    case op_ladd: {
        auto item2 = stackTopAccessW<jlong>(true);
        auto item = stackTopAccessW<jlong>(true);
        stackPushAccessW<jlong>(item2 + item);
        currentThread.pc++;
        goto operand;
    }
    case op_fadd: {
        auto i1 = stackTopAccess<jfloat>(true);
        auto i2 = stackTopAccess<jfloat>(true);
        stackPushAccess<jfloat>(i1 + i2);
        currentThread.pc++;
        goto operand;
    }
    case op_dadd: {
        auto item2 = stackTopAccessW<jdouble>(true);
        auto item = stackTopAccessW<jdouble>(true);
        stackPushAccessW<jdouble>(item2 + item);
        currentThread.pc++;
        goto operand;
    }
    case op_isub: {
        auto item2 = stackTopAccess<jint>(true);
        auto item = stackTopAccess<jint>(true);
        stackPushAccess<jint>(item - item2);
        currentThread.pc++;
        goto operand;
    }
    case op_lsub: {
        auto item2 = stackTopAccessW<jlong>(true);
        auto item = stackTopAccessW<jlong>(true);
        stackPushAccessW<jlong>(item - item2);
        currentThread.pc++;
        goto operand;
    }
    case op_fsub: {
        auto item2 = stackTopAccess<jfloat>(true);
        auto item = stackTopAccess<jfloat>(true);
        stackPushAccess<jfloat>(item - item2);
        currentThread.pc++;
        goto operand;
    }
    case op_dsub: {
        auto item2 = stackTopAccessW<jdouble>(true);
        auto item = stackTopAccessW<jdouble>(true);
        stackPushAccessW<jdouble>(item - item2);
        currentThread.pc++;
        goto operand;
    }
    case op_imul: {
        auto item2 = stackTopAccess<jint>(true);
        auto item = stackTopAccess<jint>(true);
        stackPushAccess<jint>(item * item2);
        currentThread.pc++;
        goto operand;
    }
    case op_lmul: {
        auto item2 = stackTopAccessW<jlong>(true);
        auto item = stackTopAccessW<jlong>(true);
        stackPushAccessW<jlong>(item * item2);
        currentThread.pc++;
        goto operand;
    }
    case op_fmul: {
        auto item2 = stackTopAccess<jfloat>(true);
        auto item = stackTopAccess<jfloat>(true);
        stackPushAccess<jfloat>(item * item2);
        currentThread.pc++;
        goto operand;
    }
    case op_dmul: {
        auto item2 = stackTopAccessW<jdouble>(true);
        auto item = stackTopAccessW<jdouble>(true);
        stackPushAccessW<jdouble>(item * item2);
        currentThread.pc++;
        goto operand;
    }
    case op_idiv: {
        auto item2 = stackTopAccess<jint>(true);
        auto item = stackTopAccess<jint>(true);
        stackPushAccess<jint>(item / item2);
        currentThread.pc++;
        goto operand;
    }
    case op_ldiv: {
        auto item2 = stackTopAccessW<jlong>(true);
        auto item = stackTopAccessW<jlong>(true);
        stackPushAccessW<jlong>(item / item2);
        currentThread.pc++;
        goto operand;
    }
    case op_fdiv: {
        auto item2 = stackTopAccess<jfloat>(true);
        auto item = stackTopAccess<jfloat>(true);
        stackPushAccess<jfloat>(item / item2);
        currentThread.pc++;
        goto operand;
    }
    case op_ddiv: {
        auto item2 = stackTopAccessW<jdouble>(true);
        auto item = stackTopAccessW<jdouble>(true);
        stackPushAccessW<jdouble>(item / item2);
        currentThread.pc++;
        goto operand;
    }
    case op_iinc: {
        localAccessMod<jint>(currentThread.pc[1], localAccessValue<jint>(currentThread.pc[1]) + currentThread.pc[2]);
        currentThread.pc += 3;
        goto operand;
    }
    case op_i2l: {
        stackPushAccessW<jlong>(stackTopAccess<jint>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_i2f: {
        stackPushAccess<jfloat>(stackTopAccess<jint>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_i2d: {
        stackPushAccessW<jdouble>(stackTopAccess<jint>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_l2i: {
        stackPushAccess<jint>(stackTopAccessW<jlong>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_l2f: {
        stackPushAccess<jfloat>(stackTopAccessW<jlong>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_l2d: {
        stackPushAccessW<jdouble>(stackTopAccessW<jlong>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_f2i: {
        stackPushAccess<jint>(stackTopAccess<jfloat>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_f2l: {
        stackPushAccessW<jlong>(stackTopAccess<jfloat>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_d2i: {
        stackPushAccess<jint>(stackTopAccessW<jdouble>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_f2d: {
        stackPushAccessW<jdouble>(stackTopAccess<jfloat>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_d2l: {
        stackPushAccessW<jlong>(stackTopAccessW<jdouble>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_d2f: {
        stackPushAccess<jfloat>(stackTopAccessW<jdouble>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_i2b: {
        stackPushAccess<jint>((jint)(jbyte)stackTopAccess<jint>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_i2c: {
        stackPushAccess<jint>((jint)(jchar)stackTopAccess<jint>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_i2s: {
        stackPushAccess<jint>((jint)(jshort)stackTopAccess<jint>(true));
        currentThread.pc++;
        goto operand;
    }
    case op_lcmp: {
        auto item2 = stackTopAccessW<jlong>(true);
        auto item = stackTopAccessW<jlong>(true);
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
    case op_fcmpg: {
        auto value2 = stackTopAccess<jfloat>(true);
        auto value1 = stackTopAccess<jfloat>(true);
        if (std::isnan(value1) || std::isnan(value2) || value1 > value2)
        {
            stackPushAccess<jint>(1);
        }
        else if (value1 == value2)
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
    case op_fcmpl: {
        auto value2 = stackTopAccess<jfloat>(true);
        auto value1 = stackTopAccess<jfloat>(true);
        if (std::isnan(value1) || std::isnan(value2) || value1 < value2)
        {
            stackPushAccess<jint>(-1);
        }
        else if (value1 == value2)
        {
            stackPushAccess<jint>(0);
        }
        else
        {
            stackPushAccess<jint>(1);
        }
        currentThread.pc++;
        goto operand;
    }
    case op_dcmpg: {
        auto value2 = stackTopAccessW<jdouble>(true);
        auto value1 = stackTopAccessW<jdouble>(true);
        if (std::isnan(value1) || std::isnan(value2) || value1 > value2)
        {
            stackPushAccess<jint>(1);
        }
        else if (value1 == value2)
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
    case op_dcmpl: {
        auto value2 = stackTopAccessW<jdouble>(true);
        auto value1 = stackTopAccessW<jdouble>(true);
        if (std::isnan(value1) || std::isnan(value2) || value1 < value2)
        {
            stackPushAccess<jint>(-1);
        }
        else if (value1 == value2)
        {
            stackPushAccess<jint>(0);
        }
        else
        {
            stackPushAccess<jint>(1);
        }
        currentThread.pc++;
        goto operand;
    }

#define ifcond(op, sign)                                                                                               \
    case op: {                                                                                                         \
        auto i = stackTopAccess<jint>(true);                                                                           \
        if (i sign 0)                                                                                                  \
        {                                                                                                              \
            currentThread.pc += binary::be16SignedToNative(currentThread.pc[1], currentThread.pc[2]);                  \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            currentThread.pc += 3;                                                                                     \
        }                                                                                                              \
        goto operand;                                                                                                  \
    }

        ifcond(op_ifeq, ==);
        ifcond(op_ifne, !=);
        ifcond(op_iflt, <);
        ifcond(op_ifge, >=);
        ifcond(op_ifgt, >);
        ifcond(op_ifle, <=);

#define icmpcond(op, sign)                                                                                             \
    case op: {                                                                                                         \
        auto item = stackTopAccess<jint>(true);                                                                        \
        auto item2 = stackTopAccess<jint>(true);                                                                       \
        if (item sign item2)                                                                                           \
        {                                                                                                              \
            currentThread.pc += binary::be16SignedToNative(currentThread.pc[1], currentThread.pc[2]);                  \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            currentThread.pc += 3;                                                                                     \
        }                                                                                                              \
        goto operand;                                                                                                  \
    }

        icmpcond(op_if_icmpeq, ==);
        icmpcond(op_if_icmpne, !=);
        icmpcond(op_if_icmplt, <);
        icmpcond(op_if_icmpge, >=);
        icmpcond(op_if_icmpgt, >);
        icmpcond(op_if_icmple, <=);

#define acmpcond(op, sign)                                                                                             \
    case op: {                                                                                                         \
        auto item = stackTopAccess<void *>(true);                                                                      \
        auto item2 = stackTopAccess<void *>(true);                                                                     \
        if (item sign item2)                                                                                           \
        {                                                                                                              \
            currentThread.pc += binary::be16SignedToNative(currentThread.pc[1], currentThread.pc[2]);                  \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            currentThread.pc += 3;                                                                                     \
        }                                                                                                              \
        goto operand;                                                                                                  \
    }

        acmpcond(op_if_acmpeq, ==);
        acmpcond(op_if_acmpne, !=);

    case op_goto: {
        currentThread.pc += binary::be16SignedToNative(currentThread.pc[1], currentThread.pc[2]);
        goto operand;
    }
    case op_jsr: {
        stackPushAccess<void *>(currentThread.pc);
        currentThread.pc += binary::be16SignedToNative(currentThread.pc[1], currentThread.pc[2]);
        goto operand;
    }
    case op_ret: {
        currentThread.pc = localAccessValue<uint8_t *>(currentThread.pc[1]);
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
    case op_freturn: {
        auto ret = stackTopAccess<jfloat>();
        popLastFrame();
        stackPushAccess<jfloat>(ret);
        return EXEC_RETURN;
    }
    case op_dreturn: {
        auto ret = stackTopAccessW<jdouble>();
        popLastFrame();
        stackPushAccessW<jdouble>(ret);
        return EXEC_RETURN;
    }
    case op_areturn: {
        auto ret = stackTopAccess<void *>();
        popLastFrame();
        stackPushAccess<void *>(ret);
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

        auto r = arrkl->allocateArray(stackTopAccess<jint>(true));
        stackPushAccess<void *>(r);

        currentThread.pc += 2;
        goto operand;
    }
    case op_anewarray: {
        auto id = binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1));
        auto n = *static_cast<OMKlass **>(static_cast<void *>(frame->method->klass->constantPool + id));
        auto arrn = fmt::format("[L{};", n->name);
        tower->loader->loadClass(arrn);
        auto arrc = tower->loader->fetchClass(arrn);
        auto length = stackTopAccess<jint>(true);
        stackPushAccess<void *>(arrc->allocateArray(length));
        currentThread.pc += 3;
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
