#ifndef OM_MEM_ALLOCATOR_HPP
#define OM_MEM_ALLOCATOR_HPP

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#define defmal(id)                                                                                                     \
    auto tracedMalloc##id(size_t length) -> void *;                                                                    \
    auto tracedCalloc##id(size_t count, size_t ilength) -> void *;                                                     \
    auto tracedRealloc##id(void *p, size_t length) -> void *;                                                          \
    void tracedFree##id(void *p);

namespace openminecraft::mem::allocator
{
defmal(SDL);
defmal(GL);
defmal(Elysia);
defmal(ZLib);
defmal(ElysiaExternal);
defmal(Specs);

auto stackAlloc(size_t) -> void *;
auto pageSize() -> uint64_t;
} // namespace openminecraft::mem::allocator

#endif
