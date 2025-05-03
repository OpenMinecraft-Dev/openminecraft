#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include <cstdlib>

using namespace openminecraft::mem::castorice;

namespace openminecraft::mem::allocator
{
#define defmalr(id, tag)                                                                                               \
    void *tracedMalloc##id(size_t length)                                                                              \
    {                                                                                                                  \
        void *p = malloc(length);                                                                                      \
        rec({Allocation, p, heapSize(p), tag});                                                                        \
        return p;                                                                                                      \
    }                                                                                                                  \
    void *tracedCalloc##id(size_t count, size_t ilength)                                                               \
    {                                                                                                                  \
        void *p = calloc(count, ilength);                                                                              \
        rec({Allocation, p, heapSize(p), tag});                                                                        \
        return p;                                                                                                      \
    }                                                                                                                  \
    void *tracedRealloc##id(void *p, size_t length)                                                                    \
    {                                                                                                                  \
        if (p == nullptr)                                                                                              \
            return tracedMalloc##id(length);                                                                           \
        size_t l = heapSize(p);                                                                                        \
        void *pr = realloc(p, length);                                                                                 \
        rec({Free, p, l, 1});                                                                                          \
        rec({Allocation, pr, heapSize(pr), tag});                                                                      \
        return pr;                                                                                                     \
    }                                                                                                                  \
    void tracedFree##id(void *p)                                                                                       \
    {                                                                                                                  \
        rec({Free, p, heapSize(p), tag});                                                                              \
        free(p);                                                                                                       \
    }
defmalr(SDL, 1) defmalr(Vulkan, 2)
} // namespace openminecraft::mem::allocator

using namespace openminecraft::mem::allocator;

void *operator new(size_t size)
{
    void *p = malloc(size);
    rec({Allocation, p, heapSize(p), 0});
    return p;
}

void *operator new[](size_t size)
{
    void *p = malloc(size);
    rec({Allocation, p, heapSize(p), 0});
    return p;
}

void operator delete(void *p)
{
    rec({Free, p, heapSize(p), 0});
    free(p);
}

void operator delete[](void *p)
{
    rec({Free, p, heapSize(p), 0});
    free(p);
}