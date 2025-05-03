#ifndef OM_MEM_ALLOCATOR_HPP
#define OM_MEM_ALLOCATOR_HPP

#include <cstdlib>

#define defmal(id)                                                                                                     \
    void *tracedMalloc##id(size_t length);                                                                             \
    void *tracedCalloc##id(size_t count, size_t ilength);                                                              \
    void *tracedRealloc##id(void *p, size_t length);                                                                   \
    void tracedFree##id(void *p);

namespace openminecraft::mem::allocator
{
defmal(SDL);
defmal(Vulkan);
} // namespace openminecraft::mem::allocator

#endif