#include "openminecraft/vm/elysia/om_elysia_heap.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include <cstdint>
#include <mutex>

namespace openminecraft::vm::elysia
{
static OMElysiaHeapBlock *sortBlocks(OMElysiaHeapBlock *head)
{
    if (!head || !head->next)
    {
        return head;
    }

    auto first = head;
    auto second = head->next;
    first->next = nullptr;

    if (first->block <= second->block)
    {
        first->next = second;
        return first;
    }

    auto prev = second;
    auto cur = second->next;
    while (cur && cur->block < first->block)
    {
        prev = cur;
        cur = cur->next;
    }
    prev->next = first;
    first->next = cur;
    return second;
}

OMElysiaHeap::OMElysiaHeap(const char *id, uint64_t maxSize, float expandFactor)
    : rawHeap(mem::allocator::pageSize(), maxSize), logger("OMElysiaHeap", this), maxSize(maxSize),
      expandFactor(expandFactor)
{
    rawHeap.id = id;
    rawHeap.init();

    auto rawblk = mem::allocator::tracedCallocElysia(1, sizeof(OMElysiaHeapBlock));
    emptyBlocks = new (rawblk) OMElysiaHeapBlock{rawHeap.block, rawHeap.heapTop, nullptr};

    if (enablePtrCompress())
    {
        allocate(8);
    }
}

void *OMElysiaHeap::allocate(uint64_t objLen)
{
    objLen = align(objLen);
    std::lock_guard guard(blockMutex);
beginAlloc:
    auto blk = emptyBlocks;
    while (blk)
    {
        auto length = reinterpret_cast<uintptr_t>(blk->blockEnd) - reinterpret_cast<uintptr_t>(blk->block);
        if (length > objLen)
        {
            auto target = blk->block;
            blk->block = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(blk->block) + objLen);
            mergeBlocks();

            std::memset(target, 0x00, objLen);
            return target;
        }
        blk = blk->next;
    }

    auto oldtop = rawHeap.heapTop;
    auto append =
        align(static_cast<uint64_t>(rawHeap.currentSizeAllocated() * expandFactor), mem::allocator::pageSize());
    logger.debug("expand for more {} bytes", append);
    auto newtop = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(rawHeap.heapTop) + append);
    rawHeap.expand(newtop);

    if (emptyBlocks)
    {
        blk = emptyBlocks;
        while (blk->next)
        {
            blk = blk->next;
        }
        blk->blockEnd = newtop;
    }
    else
    {
        auto rawblk = mem::allocator::tracedCallocElysia(1, sizeof(OMElysiaHeapBlock *));

        emptyBlocks = new (rawblk) OMElysiaHeapBlock;
        emptyBlocks->block = oldtop;
        emptyBlocks->blockEnd = newtop;
        emptyBlocks->next = nullptr;
    }
    goto beginAlloc;
}

void OMElysiaHeap::iterBlocks(std::function<void(OMElysiaHeapBlock *)> f)
{
    std::lock_guard guard(blockMutex);

    auto blk = emptyBlocks;
    while (blk)
    {
        f(blk);
        blk = blk->next;
    }
}

void OMElysiaHeap::deallocate(void *ptr, uint64_t length)
{
    length = align(length);
    std::lock_guard guard(blockMutex);

    auto rawblk = mem::allocator::tracedCallocElysia(1, sizeof(OMElysiaHeapBlock));
    auto newblk = new (rawblk) OMElysiaHeapBlock;
    newblk->block = ptr;
    newblk->blockEnd = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) + length);
    newblk->next = emptyBlocks;
    emptyBlocks = sortBlocks(newblk);

    mergeBlocks();
}

void OMElysiaHeap::mergeBlocks()
{
    if (!emptyBlocks)
        return;

    OMElysiaHeapBlock *prev = nullptr;
    OMElysiaHeapBlock *curr = emptyBlocks;

    while (curr)
    {
        if (curr->block == curr->blockEnd)
        {
            OMElysiaHeapBlock *toDelete = curr;
            curr = curr->next;
            if (prev)
                prev->next = curr;
            else
                emptyBlocks = curr;
            mem::allocator::tracedFreeElysia(toDelete);
            continue;
        }

        if (curr->next && curr->blockEnd == curr->next->block)
        {
            curr->blockEnd = curr->next->blockEnd;
            OMElysiaHeapBlock *toMerge = curr->next;
            curr->next = toMerge->next;
            mem::allocator::tracedFreeElysia(toMerge);
        }

        prev = curr;
        curr = curr->next;
    }
}

OMElysiaHeap::~OMElysiaHeap()
{
    while (emptyBlocks)
    {
        auto cur = emptyBlocks->next;
        mem::allocator::tracedFreeElysia(cur);
        emptyBlocks = cur;
    }
}
} // namespace openminecraft::vm::elysia
