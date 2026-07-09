#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include <cstdlib>
#include <new>

using namespace openminecraft::mem::castorice;

namespace openminecraft::mem::allocator
{
#define defmalr(id, tag)                                                                                               \
    auto tracedMalloc##id(size_t length) -> void *                                                                     \
    {                                                                                                                  \
        void *p = malloc(length);                                                                                      \
        rec({Allocation, p, heapSize(p), tag});                                                                        \
        return p;                                                                                                      \
    }                                                                                                                  \
    auto tracedCalloc##id(size_t count, size_t ilength) -> void *                                                      \
    {                                                                                                                  \
        void *p = calloc(count, ilength);                                                                              \
        rec({Allocation, p, heapSize(p), tag});                                                                        \
        return p;                                                                                                      \
    }                                                                                                                  \
    auto tracedRealloc##id(void *p, size_t length) -> void *                                                           \
    {                                                                                                                  \
        if (p == nullptr)                                                                                              \
            return tracedMalloc##id(length);                                                                           \
        size_t l = heapSize(p);                                                                                        \
        void *pr = realloc(p, length);                                                                                 \
        rec({Free, p, l, tag});                                                                                        \
        rec({Allocation, pr, heapSize(pr), tag});                                                                      \
        return pr;                                                                                                     \
    }                                                                                                                  \
    void tracedFree##id(void *p)                                                                                       \
    {                                                                                                                  \
        rec({Free, p, heapSize(p), tag});                                                                              \
        free(p);                                                                                                       \
    }

defmalr(SDL, "sdl");
defmalr(GL, "opengl");
defmalr(Elysia, "elysia_internal");
defmalr(ZLib, "zlib");
defmalr(ElysiaExternal, "elysia_external");
defmalr(Specs, "specs");
} // namespace openminecraft::mem::allocator

using namespace openminecraft::mem::allocator;

auto operator new(size_t size) -> void *
{
    void *p = malloc(size);
    rec({Allocation, p, heapSize(p), "cpp"});
    return p;
}

auto operator new[](size_t size) -> void *
{
    void *p = malloc(size);
    rec({Allocation, p, heapSize(p), "cpp"});
    return p;
}

void operator delete(void *p) noexcept
{
    if (!p)
    {
        return;
    }
    rec({Free, p, heapSize(p), "cpp"});
    free(p);
}

void operator delete[](void *p) noexcept
{
    if (!p)
    {
        return;
    }
    rec({Free, p, heapSize(p), "cpp"});
    free(p);
}

void operator delete(void *p, size_t l) noexcept
{
    if (!p)
    {
        return;
    }
    rec({Free, p, heapSize(p), "cpp"});
    free(p);
}

void operator delete[](void *p, size_t l) noexcept
{
    if (!p)
    {
        return;
    }
    rec({Free, p, heapSize(p), "cpp"});
    free(p);
}
