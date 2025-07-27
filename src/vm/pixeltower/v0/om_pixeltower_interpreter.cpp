#include "openminecraft/vm/pixeltower/v0/om_pixeltower_interpreter.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_frame.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
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

    auto nextframe = (OMFrame *)((uint8_t *)frame->stackPointer - sizeof(OMFrame));
    nextframe->stackPointer = (jbyte *)frame->stackPointer - sizeof(OMFrame) - met->maxLocals * sizeof(void *);
    nextframe->returnAddr = currentThread.pc + 3;
    nextframe->method = met;
    nextframe->prev = frame;

    currentThread.currentFrame = nextframe;
    currentThread.pc = met->code;
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
        logger.debug("sp pointed at {}", (void *)currentThread.currentFrame->stackPointer);
        logger.debug("method metadata at {}", (void *)frame);
        break;
    }
    }
    return false;
}
} // namespace openminecraft::vm::pixeltower::v0