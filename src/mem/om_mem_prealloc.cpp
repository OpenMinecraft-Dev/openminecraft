#include "openminecraft/mem/om_mem_prealloc.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include <new>

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
uint64_t OMHeap::currentSizeAllocated()
{
    return (size_t)heapTop - (size_t)block;
}
void OMHeap::expand(void *target)
{
    if (target < block || target <= heapTop || ((size_t)target - (size_t)block) > maxSize)
    {
        logger.error("target is not valid!");
        throw std::bad_alloc();
    }

    activate(heapTop, (size_t)target - (size_t)heapTop);
    mem::castorice::rec({castorice::Allocation, heapTop, (size_t)target - (size_t)heapTop, "vmdata"});
    heapTop = target;
}
void OMHeap::shrink(void *target)
{
    if (target < block || target >= heapTop)
    {
        logger.error("target is not valid!");
        throw std::bad_alloc();
    }

    deactivate(target, (size_t)heapTop - (size_t)target);
    mem::castorice::rec({castorice::Free, target, (size_t)heapTop - (size_t)target, "vmdata"});
    heapTop = target;
}
} // namespace openminecraft::mem
