#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_prealloc.hpp"
#ifdef OM_PLATFORM_BSD
#include <stdlib.h>
#else
#include <alloca.h>
#endif

#include <cerrno>
#include <cstring>
#include <new>
#include <unistd.h>
#if defined(OM_PLATFORM_IOS) || defined(OM_PLATFORM_MACOS)
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

#include <sys/mman.h>

#include <cstddef>
#include <cstdint>
#include <pthread.h>

namespace openminecraft::mem
{
OMHeap::OMHeap(uint64_t minSize, uint64_t maxSize)
    : heapTop(nullptr), logger("OMHeap", this), minSize(minSize), maxSize(maxSize)
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
    this->shrink(block);
    munmap(block, maxSize);
}

void OMHeap::activate(void *p, uint64_t length)
{
    if (madvise(p, length, MADV_WILLNEED) == -1)
    {
        logger.error("[unix-like] madvise fail ({})", strerror(errno));
        throw std::bad_alloc();
    }

    if (mprotect(p, length, PROT_READ | PROT_WRITE) == -1)
    {
        logger.error("[unix-like] mprotect fail ({}), pagesize = {}", strerror(errno), sysconf(_SC_PAGESIZE));
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

namespace allocator
{
void *stackAlloc(size_t l)
{
    return alloca(l);
}
} // namespace allocator

namespace stack
{
uintptr_t fetchStackBase()
{
#ifdef OM_PLATFORM_BSDLIKE
    void *stackTop = pthread_get_stackaddr_np(pthread_self());
    size_t stackLength = pthread_get_stacksize_np(pthread_self());
#else
    pthread_attr_t attr;
    pthread_getattr_np(pthread_self(), &attr);

    void *stackTop;
    size_t stackLength;
    pthread_attr_getstack(&attr, &stackTop, &stackLength);
    pthread_attr_destroy(&attr);
#endif

    return reinterpret_cast<uintptr_t>(stackTop) + stackLength;
}
uintptr_t fetchStackTop()
{
#ifdef OM_PLATFORM_BSDLIKE
    void *stackTop = pthread_get_stackaddr_np(pthread_self());
#else
    pthread_attr_t attr;
    pthread_getattr_np(pthread_self(), &attr);

    void *stackTop;
    size_t stackLength;
    pthread_attr_getstack(&attr, &stackTop, &stackLength);
    pthread_attr_destroy(&attr);
#endif

    return reinterpret_cast<uintptr_t>(stackTop);
}
} // namespace stack

} // namespace openminecraft::mem
