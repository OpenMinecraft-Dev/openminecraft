#include "openminecraft/vm/elysia/om_elysia_heap.hpp"

namespace openminecraft::vm::elysia
{
OMElysiaHeap::OMElysiaHeap(const char *id, uint64_t maxSize) : rawHeap(1024, maxSize)
{
    rawHeap.id = id;
    rawHeap.init();

    emptyBlocks = new OMElysiaHeapBlock{rawHeap.block, maxSize, nullptr};
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
