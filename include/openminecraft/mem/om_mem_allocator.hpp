#ifndef OM_MEM_ALLOCATOR_HPP
#define OM_MEM_ALLOCATOR_HPP

#include <cstdlib>

namespace openminecraft::mem::allocator {
void* tracedMalloc(size_t length);
void* tracedCalloc(size_t count, size_t ilength);
void* tracedRealloc(void* p, size_t length);
void tracedFree(void* p);
}

#endif