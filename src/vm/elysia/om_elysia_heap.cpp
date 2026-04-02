#include "openminecraft/vm/elysia/om_elysia_heap.hpp"
#include <iostream>

namespace openminecraft::vm::elysia
{
OMElysiaHeap::OMElysiaHeap(std::string id, uint64_t maxSize) : rawHeap(1024 * 1024, maxSize)
{
    rawHeap.id = id.c_str();

    emptyBlocks = new OMElysiaHeapBlock{rawHeap.block, maxSize, nullptr};
}

OMElysiaHeap::~OMElysiaHeap()
{
    while (emptyBlocks)
    {
        std::cout << emptyBlocks << std::endl;
        auto cur = emptyBlocks->next;
        delete cur;
        emptyBlocks = cur;
    }
}
} // namespace openminecraft::vm::elysia
