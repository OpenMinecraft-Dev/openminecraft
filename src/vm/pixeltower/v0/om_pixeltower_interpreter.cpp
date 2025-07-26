#include "openminecraft/vm/pixeltower/v0/om_pixeltower_interpreter.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_threads.hpp"

namespace openminecraft::vm::pixeltower::v0
{
OMInterpreter::OMInterpreter(OMPixelTowerHeap *heap) : heap(heap), logger("OMInterpreter", this)
{
}
OMInterpreter::~OMInterpreter()
{
}
bool OMInterpreter::execute()
{
    logger.debug("pc pointed at {}", (void *)currentThread.pc);
    logger.debug("method metadata at {}", (void *)currentThread.currentFrame);
    return false;
}
} // namespace openminecraft::vm::pixeltower::v0