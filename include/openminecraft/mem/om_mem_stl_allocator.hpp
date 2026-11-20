#ifndef OM_MEM_STL_ALLOCATOR_HPP
#define OM_MEM_STL_ALLOCATOR_HPP

#include "openminecraft/mem/om_mem_record.hpp"
#include <cstdlib>
#include <memory>
#include <string>

namespace openminecraft::mem
{
template <const char *G, typename T> class OMStlAllocator
{
  public:
    using value_type = T;

    template <typename U> struct rebind
    {
        using other = OMStlAllocator<G, U>;
    };

    OMStlAllocator() = default;

    template <const char *H, typename U> OMStlAllocator(const OMStlAllocator<H, U> &)
    {
    }

    auto allocate(std::size_t n) -> T *
    {
        auto r = static_cast<T *>(calloc(sizeof(T), n));
        castorice::rec({castorice::Allocation, r, n, G});
        return r;
    }

    void deallocate(T *p, std::size_t n) noexcept
    {
        castorice::rec({castorice::Free, p, n, G});
        free(p);
    }

  private:
    std::string id;
};

template <const char *G, typename T, typename... Args> auto fast_shared(Args &&...args) -> std::shared_ptr<T>
{
    return std::allocate_shared<T>(OMStlAllocator<G, T>(), args...);
}
}; // namespace openminecraft::mem

#endif
