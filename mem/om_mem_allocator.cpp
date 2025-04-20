#include "openminecraft/mem/om_mem_record.hpp"
#include <cstdlib>
#include <malloc.h>

namespace openminecraft::mem::allocator {
void test() { }
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