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
        logger.info("stack element ({}) arg #{}", (void *)(&stackTopPointer), i);
        logger.info("{}", stackTopPointer);
        *(void **)localAccessForeign(nextframe, i) = stackTopPointer;
        stackPop;
    }
    debugger.debugStack();

    nextframe->returnAddr = currentThread.pc + 3;
    nextframe->method = met;
    nextframe->prev = frame;

    currentThread.stackPointer = (jbyte *)nextframe - met->maxLocals * sizeof(void *);
    currentThread.currentFrame = nextframe;
    stackPush;
    currentThread.pc = met->code;
    debugger.debugStack();
}

bool OMInterpreter::execute()
{
    auto frame = currentThread.currentFrame;
    switch (currentThread.pc[0])
    {
    case op_dup: {
        stackPushPointer(stackTopPointer);
        currentThread.pc++;
        return true;
    }
    case op_return: {
        currentThread.pc = (uint8_t *)currentThread.currentFrame->returnAddr;
        currentThread.stackPointer = (uint8_t *)currentThread.currentFrame + sizeof(OMFrame); // popped whole frame
        stackPush;
        currentThread.currentFrame = currentThread.currentFrame->prev;

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
