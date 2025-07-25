#include "openminecraft/mem/om_mem_prealloc.hpp"

namespace openminecraft::mem
{
void OMHeap::init()
{
    heapTop = block;
    if (minSize == 0)
    {
        return;
    }
    expand(static_cast<uint8_t *>(block) + minSize);
}
void OMHeap::expand(void *target)
{
    if (target < block || target <= heapTop)
    {
        logger.error("target is not valid!");
    }

    activate(heapTop, (size_t)target - (size_t)heapTop);
    heapTop = target;
}
void OMHeap::shrink(void *target)
{
    if (target < block || target >= heapTop)
    {
        logger.error("target is not valid!");
    }

    deactivate(target, (size_t)heapTop - (size_t)target);
    heapTop = target;
}
} // namespace openminecraft::mem