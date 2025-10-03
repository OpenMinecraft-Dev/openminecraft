#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_prealloc.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include <cerrno>
#include <cstring>
#include <new>
#if defined(OM_PLATFORM_IOS) || defined(OM_PLATFORM_MACOS)
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

#include <sys/mman.h>

namespace openminecraft::mem
{
OMHeap::OMHeap(uint64_t minSize, uint64_t maxSize) : heapTop(nullptr), logger("OMHeap", this), minSize(minSize), maxSize(maxSize)
{
    block = mmap(nullptr, maxSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (block == nullptr)
    {
        logger.error("[unix-like] unable to map memory ({})", strerror(errno));
        throw std::bad_alloc();
    }
}

OMHeap::~OMHeap()
{
    munmap(block, maxSize);
}

void OMHeap::activate(void *p, uint64_t length)
{
    if (mprotect(p, length, PROT_READ | PROT_WRITE) == -1)
    {
        logger.error("[unix-like] mprotect fail ({})", strerror(errno));
        throw std::bad_alloc();
    }
}

void OMHeap::deactivate(void *p, uint64_t length)
{
    if (madvise(p, length, MADV_DONTNEED) == -1)
    {
        logger.error("[unix-like] madvise fail ({})", strerror(errno));
        throw std::bad_alloc();
    }

    if (mprotect(p, length, PROT_NONE) == -1)
    {
        logger.error("[unix-like] mprotect fail ({})", strerror(errno));
        throw std::bad_alloc();
    }
}
} // namespace openminecraft::mem

namespace openminecraft::mem::castorice
{
size_t heapSize(void *p)
{
#if defined(OM_PLATFORM_IOS) || defined(OM_PLATFORM_MACOS)
    return malloc_size(p);
#else
    return malloc_usable_size(p);
#endif
}
} // namespace openminecraft::mem::castorice