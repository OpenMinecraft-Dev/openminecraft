#include "openminecraft/mem/om_mem_prealloc.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include <errhandlingapi.h>
#include <iostream>
#include <malloc.h>
#include <memoryapi.h>
#include <new>
#include <oleauto.h>
#include <windows.h>
#include <winnt.h>

namespace openminecraft::mem
{
OMHeap::OMHeap(uint64_t minSize, uint64_t maxSize) : logger("OMHeap", this), maxSize(maxSize), minSize(minSize)
{
    block = VirtualAlloc(nullptr, maxSize, MEM_RESERVE, PAGE_NOACCESS);
    if (block == nullptr)
    {
        logger.error("[windows] VirtualAlloc fail ({})", GetLastError());
        throw std::bad_alloc();
    }
}

OMHeap::~OMHeap()
{
    this->shrink(block);
    VirtualFree(block, 0, MEM_RELEASE);
}

void OMHeap::activate(void *p, uint64_t length)
{
    if (!VirtualAlloc(p, length, MEM_COMMIT, PAGE_READWRITE))
    {
        logger.error("[windows] VirtualAlloc fail ({})", GetLastError());
        throw std::bad_alloc();
    }
}

void OMHeap::deactivate(void *p, uint64_t length)
{
    if (!VirtualFree(p, length, MEM_DECOMMIT))
    {
        logger.error("[windows] VirtualFree fail ({})", GetLastError());
        throw std::bad_alloc();
    }
}

void *stackAlloc(size_t l)
{
    return _alloca(l);
}
} // namespace openminecraft::mem
