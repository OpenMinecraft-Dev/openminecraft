#include "openminecraft/vm/pixeltower/v0/om_pixeltower_interpreter.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_field.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_frame.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_oop.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_threads.hpp"
#include <cstring>
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

void OMInterpreter::call(OMMethod *met)
{
    if (!currentThread.currentFrame)
    {
        auto frame = (OMFrame *)((uint8_t *)currentThread.stackPointer - sizeof(OMFrame));
        currentThread.stackPointer = (jbyte *)currentThread.stackPointer - sizeof(OMFrame) -
                                     met->maxLocals * sizeof(void *); // allocate whole frame + locals
        stackPush();
        frame->returnAddr = (void *)0x33550336;
        frame->prev = nullptr;
        frame->method = met;

        currentThread.currentFrame = frame;
        currentThread.pc = met->code;
        return;
    }

    auto frame = currentThread.currentFrame;
    auto nextframe = (OMFrame *)((uint8_t *)currentThread.stackPointer - sizeof(OMFrame) + sizeof(void *) +
                                 met->args * sizeof(void *));
    nextframe->method = met;

    std::vector<void *> args(met->args);
    for (int i = met->args - 1; i >= 0; i--)
    {
        args[i] = stackTopAccess<void *>();
        stackPop();
    }

    for (int i = 0; i < met->maxLocals; i++)
    {
        *localAccess<void *>(i, nextframe) = nullptr;
    }
    for (int i = 0; i < args.size(); i++)
    {
        *localAccess<void *>(i, nextframe) = args[i];
    }

    nextframe->returnAddr = currentThread.pc + 3;
    nextframe->prev = frame;

    currentThread.stackPointer = (jbyte *)nextframe - met->maxLocals * sizeof(void *);
    currentThread.currentFrame = nextframe;
    stackPush();
    currentThread.pc = met->code;
}

void OMInterpreter::callDynamic(OMMethod *met)
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

    OMMethod *codetarget = met;
    auto cls = static_cast<OMOOPDesc *>(args[0])->klass;
    while (cls != nullptr)
    {
        auto m = cls->methods;
        while (m != nullptr)
        {
            if (strcmp(m->name, met->name) == 0 && strcmp(m->desc, m->desc) == 0)
            {
                codetarget = m;
                goto endSea;
            }
            m = m->next;
        }
        cls = cls->superClass;
    }
endSea:
    nextframe->method = codetarget;

    for (int i = 0; i < met->maxLocals; i++)
    {
        *localAccess<void *>(i, nextframe) = nullptr;
    }
    for (int i = 0; i < args.size(); i++)
    {
        *localAccess<void *>(i, nextframe) = args[i];
    }

    nextframe->returnAddr = currentThread.pc + 3;
    nextframe->prev = frame;

    currentThread.stackPointer = (jbyte *)nextframe - nextframe->method->maxLocals * sizeof(void *);
    currentThread.currentFrame = nextframe;
    stackPush();
    currentThread.pc = codetarget->code;
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
        return 0;
    }
    case op_lconst_l(0):
    case op_lconst_l(1): {
        stackPushAccessW<jlong>((jlong)currentThread.pc[0] - (jlong)op_lconst_l(0));
        currentThread.pc++;
        return 0;
    }
    case op_iload_n(0):
    case op_iload_n(1):
    case op_iload_n(2):
    case op_iload_n(3): {
        stackPushAccess<jint>(localAccessValue<jint>(currentThread.pc[0] - op_iload_n(0)));
        currentThread.pc++;
        return 0;
    }
    case op_lload_n(0):
    case op_lload_n(1):
    case op_lload_n(2):
    case op_lload_n(3): {
        stackPushAccessW<jlong>(localAccessValueW<jlong>(currentThread.pc[0] - op_lload_n(0)));
        currentThread.pc++;
        return 0;
    }
    case op_aload_n(0):
    case op_aload_n(1):
    case op_aload_n(2):
    case op_aload_n(3): {
        stackPushAccess<void *>(localAccessValue<void *>(currentThread.pc[0] - op_aload_n(0)));
        currentThread.pc++;
        return 0;
    }
    case op_istore_n(0):
    case op_istore_n(1):
    case op_istore_n(2):
    case op_istore_n(3): {
        *localAccess<jint>(currentThread.pc[0] - op_istore_n(0)) = stackTopAccess<jint>();
        stackPop();
        currentThread.pc++;
        return 0;
    }
    case op_astore_n(0):
    case op_astore_n(1):
    case op_astore_n(2):
    case op_astore_n(3): {
        *localAccess<void *>(currentThread.pc[0] - op_astore_n(0)) = stackTopAccess<void *>();
        stackPop();
        currentThread.pc++;
        return 0;
    }
    case op_pop: {
        stackPop();
        currentThread.pc++;
        return 0;
    }
    case op_pop2: {
        stackPopW();
        currentThread.pc++;
        return 0;
    }
    case op_dup: {
        stackPushAccess<void *>(stackTopAccess<void *>());
        currentThread.pc++;
        return 0;
    }
    case op_ladd: {
        auto item2 = stackTopAccessW<jlong>();
        stackPopW();
        auto item = stackTopAccessW<jlong>();
        stackPopW();
        stackPushAccessW<jlong>(item2 + item);
        currentThread.pc++;
        return 0;
    }
    case op_i2l: {
        auto v = stackTopAccess<jint>();
        stackPop();
        stackPushAccessW<jlong>(v);
        currentThread.pc++;
        return 0;
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
        return 0;
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
        return 0;
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

        return 0;
    }
    case op_ireturn: {
        auto ret = stackTopAccess<jint>();
        popLastFrame();
        stackPushAccess<jint>(ret);
        return 0;
    }
    case op_lreturn: {
        auto ret = stackTopAccessW<jlong>();
        popLastFrame();
        stackPushAccessW<jlong>(ret);
        return 0;
    }
    case op_return: {
        bool isClinit = strcmp("<clinit>", currentThread.currentFrame->method->name) == 0;
        popLastFrame();
        return isClinit ? 2 : 0;
    }
    case op_putfield: {
        auto n = *static_cast<OMField **>(static_cast<void *>(
            frame->method->klass->constantPool + binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1))));
        accessField(n);
        currentThread.pc += 3;

        return 0;
    }
    case op_invokevirtual: {
        auto n = *static_cast<OMMethod **>(static_cast<void *>(
            frame->method->klass->constantPool + binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1))));
        callDynamic(n);

        return 0;
    }
    case op_invokespecial: {
        auto n = *static_cast<OMMethod **>(static_cast<void *>(
            frame->method->klass->constantPool + binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1))));
        call(n);

        return 0;
    }
    case op_new: {
        auto n = *static_cast<OMKlass **>(static_cast<void *>(
            frame->method->klass->constantPool + binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1))));
        stackPushAccess<void *>(n->allocateInstance());
        currentThread.pc += 3;
        return 0;
    }
    default: {
    endExec:
        logger.debug("unknown operand at {}", (void *)currentThread.pc);
        logger.debug("thread {}", (void *)&currentThread.id);
        logger.debug("pc pointed at {} ({}.{}{} + {})", (void *)currentThread.pc,
                     currentThread.currentFrame->method->klass->name, currentThread.currentFrame->method->name,
                     currentThread.currentFrame->method->desc,
                     reinterpret_cast<size_t>(
                         reinterpret_cast<void *>(static_cast<uint8_t *>(currentThread.pc) -
                                                  static_cast<uint8_t *>(currentThread.currentFrame->method->code))));

        debugger.debugStack();

        break;
    }
    }
    return 1;
}
} // namespace openminecraft::vm::pixeltower::v0
