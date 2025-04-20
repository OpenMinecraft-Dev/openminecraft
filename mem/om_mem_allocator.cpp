#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include <cstdlib>

namespace openminecraft::mem::allocator {
void* tracedMalloc(size_t length)
{
    void* p = malloc(length);
    rec({ Allocation, p, heapSize(p), 1 });
    return p;
}

void* tracedCalloc(size_t count, size_t ilength)
{
    void* p = calloc(count, ilength);
    rec({ Allocation, p, heapSize(p), 1 });
    return p;
}

void* tracedRealloc(void* p, size_t length)
{
    void* pr = realloc(p, length);
    rec({ Free, p, heapSize(p), 1 });
    rec({ Allocation, pr, heapSize(pr), 1 });
    return pr;
}

void tracedFree(void* p)
{
    rec({ Free, p, heapSize(p), 1 });
    free(p);
}
}

using namespace openminecraft::mem::allocator;

void* operator new(size_t size)
{
    void* p = malloc(size);
    rec({ Allocation, p, heapSize(p), 0 });
    return p;
}

void* operator new[](size_t size)
{
    void* p = malloc(size);
    rec({ Allocation, p, heapSize(p), 0 });
    return p;
}

void operator delete(void* p)
{
    rec({ Free, p, heapSize(p), 0 });
    free(p);
}

void operator delete[](void* p)
{
    rec({ Free, p, heapSize(p), 0 });
    free(p);
}