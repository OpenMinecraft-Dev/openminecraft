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
    rec({ Allocation, p, malloc_usable_size(p), 0 });
    return p;
}

void* operator new[](size_t size)
{
    void* p = malloc(size);
    rec({ Allocation, p, malloc_usable_size(p), 0 });
    return p;
}

void operator delete(void* p)
{
    rec({ Free, p, malloc_usable_size(p), 0 });
    free(p);
}

void operator delete[](void* p)
{
    rec({ Free, p, malloc_usable_size(p), 0 });
    free(p);
}