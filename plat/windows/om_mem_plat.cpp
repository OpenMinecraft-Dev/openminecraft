#include "openminecraft/mem/om_mem_prealloc.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include <malloc.h>

namespace openminecraft::mem
{
OMHeap::OMHeap(uint64_t minSize, uint64_t maxSize) : logger("OMHeap", this), maxSize(maxSize), minSize(minSize)
{
}

OMHeap::~OMHeap()
{
}

void OMHeap::activate(void *p, uint64_t length)
{
}

void OMHeap::deactivate(void *p, uint64_t length)
{
}
} // namespace openminecraft::mem

namespace openminecraft::mem::castorice
{
size_t heapSize(void *p)
{
    return _msize(p);
}
} // namespace openminecraft::mem::castorice