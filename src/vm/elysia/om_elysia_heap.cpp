#include "openminecraft/vm/elysia/om_elysia_heap.hpp"
#include <cstdint>
#include <mutex>

namespace openminecraft::vm::elysia
{
/*static OMElysiaHeapBlock *sortBlocksMerge(OMElysiaHeapBlock *l1, OMElysiaHeapBlock *l2)
{
    OMElysiaHeapBlock dummy = {nullptr, nullptr, nullptr};
    OMElysiaHeapBlock *tail = &dummy;
    while (l1 && l2)
    {
        if (l1->block <= l2->block)
        {
            tail->next = l1;
            l1 = l1->next;
        }
        else
        {
            tail->next = l2;
            l2 = l2->next;
        }
        tail = tail->next;
    }
    tail->next = l1 ? l1 : l2;
    return dummy.next;
}
static OMElysiaHeapBlock *sortBlocks(OMElysiaHeapBlock *head)
{
    if (!head || !head->next)
    {
        return head;
    }

    auto slow = head, fast = head->next;
    while (fast && fast->next)
    {
        slow = slow->next;
        fast = fast->next->next;
    }
    auto mid = slow->next;
    slow->next = nullptr;

    auto left = sortBlocks(head);
    auto right = sortBlocks(mid);

    return sortBlocksMerge(left, right);
}*/

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

    auto blk = emptyBlocks;
    while (blk)
    {
        logger.warn("{} {}", blk->block, blk->blockEnd);
        blk = blk->next;
    }
    throw 0;
}

void OMElysiaHeap::mergeBlocks()
{
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
