#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include <cstdlib>
#include <new>

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

defmalr(SDL, OM_MEM_SDL);
defmalr(Freetype, OM_MEM_FREETYPE);
defmalr(Harfbuzz, OM_MEM_HARFBUZZ);
defmalr(GL, OM_MEM_OPENGL);
defmalr(VMData, OM_MEM_VMDATA);
} // namespace openminecraft::mem::allocator

using namespace openminecraft::mem::allocator;

void *operator new(size_t size)
{
    void *p = malloc(size);
    rec({Allocation, p, heapSize(p), OM_MEM_CPP});
    return p;
}

void *operator new[](size_t size)
{
    void *p = malloc(size);
    rec({Allocation, p, heapSize(p), OM_MEM_CPP});
    return p;
}

void operator delete(void *p) noexcept
{
    rec({Free, p, heapSize(p), OM_MEM_CPP});
    free(p);
}

void operator delete[](void *p) noexcept
{
    rec({Free, p, heapSize(p), OM_MEM_CPP});
    free(p);
}

void operator delete(void *p, size_t l) noexcept
{
    rec({Free, p, l, OM_MEM_CPP});
    free(p);
}

void operator delete[](void *p, size_t l) noexcept
{
    rec({Free, p, l, OM_MEM_CPP});
    free(p);
}
