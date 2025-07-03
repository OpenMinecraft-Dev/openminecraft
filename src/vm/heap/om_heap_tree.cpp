#include "openminecraft/vm/heap/om_heap_tree.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include <algorithm>
#include <vector>

namespace openminecraft::vm::heap
{
OMHeapTree::OMHeapTree() : logger("OMHeapTree", this)
{
}

OMHeapTree::~OMHeapTree()
{
    for (auto pair : data)
    {
        logger.info("Deconstructing memory block {} (#{})", pair.second, pair.first);
        mem::allocator::tracedFreeVMData(pair.second);
    }
}

void OMHeapTree::attach(uint64_t from, uint64_t to)
{
    refs[from].push_back(to);
}

void OMHeapTree::checkUnreachable(std::vector<uint64_t> *target, int id)
{
    if (!std::count(target->begin(), target->end(), id))
    {
        target->push_back(id);
        for (auto a : refs[id])
        {
            checkUnreachable(target, a);
        }
    }
}

void OMHeapTree::allocate(uint64_t id, uint64_t length)
{
    data[id] = mem::allocator::tracedMallocVMData(length);
}
}; // namespace openminecraft::vm::heap