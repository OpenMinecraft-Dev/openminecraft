#include "openminecraft/vm/pixeltower/v0/om_pixeltower_interpreter.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_frame.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_oop.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_threads.hpp"
#include <cstring>

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
    auto frame = currentThread.currentFrame;
    auto nextframe = (OMFrame *)((uint8_t *)currentThread.stackPointer - sizeof(OMFrame) + sizeof(void *) +
                                 met->args * sizeof(void *));
    for (int i = 0; i < met->maxLocals; i++)
    {
        *(void **)localAccessForeign(nextframe, i) = nullptr;
    }
    for (int i = 0; i < met->args; i++)
    {
        *(void **)localAccessForeign(nextframe, i) = stackTopPointer;
        stackPop;
    }

    nextframe->returnAddr = currentThread.pc + 3;
    nextframe->method = met;
    nextframe->prev = frame;

    currentThread.stackPointer = (jbyte *)nextframe - met->maxLocals * sizeof(void *);
    currentThread.currentFrame = nextframe;
    stackPush;
    currentThread.pc = met->code;
}

void OMInterpreter::callDynamic(OMMethod *met)
{
    auto frame = currentThread.currentFrame;
    auto nextframe = (OMFrame *)((uint8_t *)currentThread.stackPointer - sizeof(OMFrame) + sizeof(void *) +
                                 met->args * sizeof(void *));
    for (int i = 0; i < met->maxLocals; i++)
    {
        *(void **)localAccessForeign(nextframe, i) = nullptr;
    }

    uint8_t *codetarget = met->code;
    for (int i = 0; i < met->args; i++)
    {
        *(void **)localAccessForeign(nextframe, i) = stackTopPointer;
        if (i == met->args - 1)
        {
            auto cls = static_cast<OMOOPDesc *>(stackTopPointer)->klass;
            while (cls != nullptr)
            {
                auto m = cls->methods;
                while (m != nullptr)
                {
                    if (strcmp(m->name, met->name) == 0 && strcmp(m->desc, m->desc) == 0)
                    {
                        codetarget = m->code;
                        goto endSea;
                    }
                    m = m->next;
                }
                cls = cls->superClass;
            }
            logger.debug("dyn object {}", stackTopPointer);
        }
    endSea:
        stackPop;
    }

    nextframe->returnAddr = currentThread.pc + 3;
    nextframe->method = met;
    nextframe->prev = frame;

    currentThread.stackPointer = (jbyte *)nextframe - met->maxLocals * sizeof(void *);
    currentThread.currentFrame = nextframe;
    stackPush;
    currentThread.pc = met->code;
}

void OMInterpreter::popLastFrame()
{
    currentThread.pc = (uint8_t *)currentThread.currentFrame->returnAddr;
    currentThread.stackPointer = (uint8_t *)currentThread.currentFrame + sizeof(OMFrame); // popped whole frame
    currentThread.currentFrame = currentThread.currentFrame->prev;
    stackPush;
}

bool OMInterpreter::execute()
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
        stackPushInt(currentThread.pc[0] - op_iconst_i(0));
        currentThread.pc++;
        return true;
    }
    case op_aload_n(0):
    case op_aload_n(1):
    case op_aload_n(2):
    case op_aload_n(3): {
        stackPushPointer(*(void **)localAccess(currentThread.pc[0] - op_aload_n(0)));
        currentThread.pc++;
        return true;
    }
    case op_pop: {
        stackPop;
        currentThread.pc++;
        return true;
    }
    case op_dup: {
        stackPushPointer(stackTopPointer);
        currentThread.pc++;
        return true;
    }
    case op_if_acmpne: {
        auto item = stackTopPointer;
        stackPop;
        auto item2 = stackTopPointer;
        stackPop;
        if (item != item2)
        {
            currentThread.pc += binary::be16SignedToNative(currentThread.pc[1], currentThread.pc[2]);
        }
        else
        {
            currentThread.pc += 3;
        }

        return true;
    }
    case op_ireturn: {
        auto ret = stackTopInt;
        popLastFrame();
        stackPushInt(ret);
        return true;
    }
    case op_return: {
        popLastFrame();
        return true;
    }
    case op_invokevirtual: {
        auto n = *static_cast<OMMethod **>(static_cast<void *>(
            frame->method->klass->constantPool + binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1))));
        callDynamic(n);

        return true;
    }
    case op_invokespecial: {
        auto n = *static_cast<OMMethod **>(static_cast<void *>(
            frame->method->klass->constantPool + binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1))));
        call(n);

        return true;
    }
    case op_new: {
        auto n = *static_cast<OMKlass **>(static_cast<void *>(
            frame->method->klass->constantPool + binary::be16ToNative(*(uint16_t *)(currentThread.pc + 1))));
        stackPushPointer(n->allocateInstance());
        currentThread.pc += 3;
        return true;
    }
    default: {
        logger.debug("unknown command at {}", (void *)currentThread.pc);
        logger.debug("thread {}", (void *)&currentThread.id);
        logger.debug("pc pointed at {}", (void *)currentThread.pc);
        logger.debug("sp pointed at {}", (void *)currentThread.stackPointer);
        logger.debug("method metadata at {}", (void *)frame);

        debugger.debugStack();

        break;
    }
    }
    return false;
}
} // namespace openminecraft::vm::pixeltower::v0
