#include "openminecraft/vm/pixeltower/v0/om_pixeltower_interpreter.hpp"
#include "fmt/color.h"
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
    auto nextframe =
        (OMFrame *)((uint8_t *)currentThread.stackPointer - sizeof(OMFrame) + (met->args * sizeof(void *)));

    for (int i = 0; i < met->args; i++)
    {
        stackPop;
        logger.info("stack element ({}) -> new frame locals ({}) arg #{}",
                    (void *)((void **)currentThread.stackPointer - 1), (void *)localAccessForeign(nextframe, i), i);
        logger.info("{}", stackTopPointer);
    }

    nextframe->returnAddr = currentThread.pc + 3;
    nextframe->method = met;
    nextframe->prev = frame;

    currentThread.stackPointer =
        (jbyte *)currentThread.stackPointer - sizeof(OMFrame) - met->maxLocals * sizeof(void *);
    stackPush;
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
        currentThread.stackPointer = (uint8_t *)currentThread.currentFrame + sizeof(OMFrame); // popped whole frame
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

        void *currentData = (uint8_t *)currentThread.stackPointer - 6 * sizeof(void *);
        while (true)
        {
            bool dirty = false;
            std::string ext = "";
            if (currentData == currentThread.stackPointer)
            {
                ext = "<== Stack pointer (dirty data)";
                dirty = true;
            }
            else if (currentData < currentThread.stackPointer)
            {
                dirty = true;
            }
            logger.debug("STACK {}: {} {}", currentData,
                         fmt::styled(*(void **)currentData, fmt::fg(dirty ? fmt::color::gray : fmt::color::white)),
                         ext);

            currentData = (uint8_t *)currentData + sizeof(void *);
            if (currentData == currentThread.stack)
            {
                break;
            }
        }

        break;
    }
    }
    return false;
}
} // namespace openminecraft::vm::pixeltower::v0