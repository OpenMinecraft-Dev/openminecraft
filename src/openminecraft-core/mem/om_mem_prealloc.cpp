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
auto OMHeap::currentSizeAllocated() -> uint64_t
{
    return reinterpret_cast<uintptr_t>(heapTop) - reinterpret_cast<uintptr_t>(block);
}
void OMHeap::expand(void *target)
{
    if (target < block || target <= heapTop ||
        (reinterpret_cast<uintptr_t>(target) - reinterpret_cast<uintptr_t>(block)) > maxSize)
    {
        logger.error("target is not valid!");
        throw std::bad_alloc();
    }

    activate(heapTop, reinterpret_cast<uintptr_t>(target) - reinterpret_cast<uintptr_t>(heapTop));
    mem::castorice::rec({castorice::Allocation, heapTop,
                         reinterpret_cast<uintptr_t>(target) - reinterpret_cast<uintptr_t>(heapTop), id});
    heapTop = target;
}
void OMHeap::shrink(void *target)
{
    if (target < block || target >= heapTop)
    {
        logger.error("target is not valid!");
        throw std::bad_alloc();
    }

    deactivate(target, reinterpret_cast<uintptr_t>(heapTop) - reinterpret_cast<uintptr_t>(target));
    mem::castorice::rec(
        {castorice::Free, target, reinterpret_cast<uintptr_t>(heapTop) - reinterpret_cast<uintptr_t>(target), id});
    heapTop = target;
}
} // namespace openminecraft::mem
