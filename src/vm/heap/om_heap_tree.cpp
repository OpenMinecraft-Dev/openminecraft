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

bool OMHeapTree::attach(uint64_t from, uint64_t to)
{
    logger.debug("attached #{} to #{}", to, from);
    for (auto pairs : refs)
    {
        if (pairs.first == from && std::count(pairs.second.begin(), pairs.second.end(), to))
        {
            logger.warn("attached!");
            return false;
        }
    }
    refs[from].push_back(to);
    return true;
}

bool OMHeapTree::detach(uint64_t from, uint64_t to)
{
    logger.debug("detached #{} to #{}", to, from);
    if (std::count(refs[from].begin(), refs[from].end(), to))
    {
        refs[from].erase(
            std::remove_if(refs[from].begin(), refs[from].end(), [to](uint64_t d) -> bool { return d == to; }),
            refs[from].end());
        return true;
    }
    else
    {
        logger.warn("not attached!");
        return false;
    }
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

void *OMHeapTree::operator[](int i)
{
    return data[i];
}

void OMHeapTree::deconstruct(uint64_t id)
{
    logger.info("Deconstructing unreachable memory block {} (#{})", data[id], id);
    mem::allocator::tracedFreeVMData(data[id]);
    data.erase(id);
}

void OMHeapTree::deconstructUnreachable()
{
    logger.debug("garbage collection!");
    std::vector<uint64_t> d;
    std::vector<uint64_t> c;
    checkUnreachable(&d);

    for (auto i : data)
    {
        if (!std::count(d.begin(), d.end(), i.first))
        {
            c.push_back(i.first);
        }
    }

    for (auto m : c)
    {
        deconstruct(m);
    }
}

bool OMHeapTree::allocate(uint64_t id, uint64_t length)
{
    for (auto pairs : data)
    {
        if (pairs.first == id)
        {
            return false;
        }
    }
    data[id] = mem::allocator::tracedMallocVMData(length);
    return true;
}
}; // namespace openminecraft::vm::heap