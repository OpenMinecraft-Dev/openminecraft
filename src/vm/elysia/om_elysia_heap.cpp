#include "openminecraft/vm/elysia/om_elysia_heap.hpp"
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

OMElysiaHeap::OMElysiaHeap(const char *id, uint64_t maxSize) : rawHeap(1024 * 4, maxSize), logger("OMElysiaHeap", this)
{
    rawHeap.id = id;
    rawHeap.init();

    emptyBlocks = new OMElysiaHeapBlock{rawHeap.block, rawHeap.heapTop, nullptr};
}

void *OMElysiaHeap::allocate(uint64_t objLen)
{
    std::lock_guard guard(blockMutex);
beginAlloc:
    auto blk = emptyBlocks;
    while (blk)
    {
        auto length = reinterpret_cast<uintptr_t>(blk->blockEnd) - reinterpret_cast<uintptr_t>(blk->block);
        if (length >= objLen)
        {
            auto target = blk->block;
            blk->block = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(blk->block) + objLen);
	    mergeBlocks();
            return target;
        }
        blk = blk->next;
    }

    auto newtop = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(rawHeap.heapTop) + 1024 * 1024);
    rawHeap.expand(newtop);

    blk = emptyBlocks;
    while (blk->next)
    {
        blk = blk->next;
    }
    blk->blockEnd = newtop;
    goto beginAlloc;
}

void OMElysiaHeap::deallocate(void *ptr, uint64_t length)
{
    std::lock_guard guard(blockMutex);

    auto newblk = new OMElysiaHeapBlock;
    newblk->block = ptr;
    newblk->blockEnd = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) + length);
    newblk->next = emptyBlocks;
    emptyBlocks = sortBlocks(newblk);

    mergeBlocks();

    auto blk = emptyBlocks;
    while (blk)
    {
        logger.warn("{} {}", blk->block, blk->blockEnd);
        blk = blk->next;
    }
    logger.error("--------------");
}

/*void OMElysiaHeap::mergeBlocks()
{
    auto blk = emptyBlocks;
    while (blk && blk->next) {
        if (blk->next->block == blk->next->blockEnd) {
	    blk->next = blk->next->next;
	}
	else if (blk->blockEnd == blk->next->block) {
	    blk->blockEnd = blk->next->blockEnd;
	    blk->next = blk->next->next;
	}
	else if (blk->blockEnd > blk->next->block) {
            throw "Memory region intersects!";
        }

	blk = blk->next;
    }

    if (emptyBlocks->block == emptyBlocks->blockEnd) {
        emptyBlocks = emptyBlocks->next;                                                            }
}*/

void OMElysiaHeap::mergeBlocks()
{
    if (!emptyBlocks) return;

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
            delete toDelete;
            continue;
        }

        if (curr->next && curr->blockEnd == curr->next->block)
        {
            curr->blockEnd = curr->next->blockEnd;
            OMElysiaHeapBlock *toMerge = curr->next;
            curr->next = toMerge->next;
            delete toMerge;
            continue;
        }

	if (curr->next && curr->blockEnd > curr->next->block) {
	    throw std::logic_error("block intersect!");
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
        delete cur;
        emptyBlocks = cur;
    }
}
} // namespace openminecraft::vm::elysia
