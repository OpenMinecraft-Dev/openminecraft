#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/mem/om_mem_prealloc.hpp"
#include <algorithm>
#include <mutex>

namespace openminecraft::vm::pixeltower::v0
{
OMPixelTowerHeap::OMPixelTowerHeap(uint64_t minSize, uint64_t maxSize) : logger("OMPixelTowerHeap", this)
{
    heap = new mem::OMHeap(minSize, maxSize);
    heap->init();
    emptyBlocks.push_back({heap->block, heap->currentSizeAllocated()});
}
OMPixelTowerHeap::~OMPixelTowerHeap()
{
    delete heap;
}

void *OMPixelTowerHeap::allocate(uint64_t length)
{
    // force 8-byte alignment
    if (length % 8 != 0)
    {
        length = length + 8 - (length % 8);
    }

alloc:
    std::lock_guard<std::mutex> g(emptyBlockMutex);
    for (auto it = emptyBlocks.begin(); it != emptyBlocks.end(); ++it)
    {
        if (it->length >= length) // available blocks found
        {
            void *target = it->ptr;
            it->ptr = static_cast<uint8_t *>(it->ptr) + length;
            it->length -= length;
            if (it->length == 0 && emptyBlocks.size() > 1)
            {
                emptyBlocks.erase(it);
            }
            return target;
        }
    }

    logger.debug("expands heap space from {} ...", heap->currentSizeAllocated());
    heap->expand(static_cast<uint8_t *>(heap->heapTop) + 1ul * 1024 * 1024); // expand, 1MB larger heap
    emptyBlocks.back().length += 1ul * 1024 * 1024;
    logger.debug("to {}", heap->currentSizeAllocated());

    goto alloc;
}
void OMPixelTowerHeap::debug()
{
    logger.info("{} bytes allocated", heap->currentSizeAllocated());
    uint64_t t = 0;
    for (auto p : emptyBlocks)
    {
        t += p.length;
    }
    logger.info("{} bytes free", t);
}
void OMPixelTowerHeap::deallocate(void *ptr, uint64_t length)
{
    if (!heap->vaild(ptr))
    {
        logger.error("{} not vaild!", ptr);
        return;
    }
    for (auto it = emptyBlocks.begin(); it != emptyBlocks.end(); ++it)
    {
        if (it->ptr > ptr)
        {
            continue;
        }

        emptyBlockMutex.lock();
        emptyBlocks.insert(it, {ptr, length});
        emptyBlockMutex.unlock();
        goto merge;
    }

    // first object
    emptyBlockMutex.lock();
    emptyBlocks.insert(emptyBlocks.begin(), {ptr, length});
    emptyBlockMutex.unlock();

merge:
    merge();
}

void OMPixelTowerHeap::merge()
{
    std::lock_guard<std::mutex> g(emptyBlockMutex);
    std::sort(emptyBlocks.begin(), emptyBlocks.end(),
              [](const OMHeapEmptyBlock &b1, const OMHeapEmptyBlock &b2) { return b1.ptr < b2.ptr; });

    auto it1 = emptyBlocks.begin();
    auto it2 = emptyBlocks.begin();
    ++it2;
    while (true)
    {
        if (it2 == emptyBlocks.end())
        {
            break;
        }

        if (static_cast<uint8_t *>(it1->ptr) + it1->length == it2->ptr) // link!
        {
            it1->length += it2->length;
            it2 = emptyBlocks.erase(it2);
        }
        else
        {
            ++it1;
            ++it2;
        }
    }
}
} // namespace openminecraft::vm::pixeltower::v0