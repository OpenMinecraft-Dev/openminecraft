#ifndef OM_ELYSIA_HEAP
#define OM_ELYSIA_HEAP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_prealloc.hpp"
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <stdexcept>

namespace openminecraft::vm::elysia
{
constexpr const char allocatorTag[] = "elysia_internal";

struct OMElysiaHeapBlock
{
    void *block;
    void *blockEnd;
    OMElysiaHeapBlock *next;
};

class OMElysiaHeap
{
  public:
    OMElysiaHeap(const char *name, uint64_t maxSize, float expandFactor);
    ~OMElysiaHeap();

    auto allocate(uint64_t length) -> void *;
    void deallocate(void *ptr, uint64_t length);
    void mergeBlocks();
    void iterBlocks(std::function<void(OMElysiaHeapBlock *)>);
    auto base() -> void *
    {
        return rawHeap.block;
    }
    auto align(uint64_t size, uint64_t alignsize = 8) -> uint64_t
    {
        return (size % alignsize) ? (size + (alignsize - size % alignsize)) : size;
    }

    template <typename T> inline auto allocate() -> T *
    {
        return reinterpret_cast<T *>(allocate(sizeof(T)));
    }
    template <typename T> inline void deallocate(T *ptr)
    {
        deallocate(ptr, sizeof(T));
    }

    template <typename T> inline auto allocateArray(uint32_t length) -> T *
    {
        return reinterpret_cast<T *>(allocate(sizeof(T) * length));
    }
    template <typename T> inline void deallocateArray(T *ptr, uint32_t length)
    {
        deallocate(ptr, sizeof(T) * length);
    }

    inline auto allocateStr(std::string s) -> char *
    {
        auto t = reinterpret_cast<char *>(allocate(s.size() + 1));
        std::strcpy(t, s.c_str());
        return t;
    }
    void deallocateStr(char *c)
    {
        deallocate(c, std::strlen(c));
    }

    inline auto enablePtrCompress() -> bool
    {
        return sizeof(void *) == 8 && maxSize < 1024ll * 1024 * 1024 * 32;
    }

    inline auto ptrLength() -> uint64_t
    {
        return enablePtrCompress() ? 4 : sizeof(void *);
    }

    inline auto compress(void *p) -> uint32_t
    {
        if (!enablePtrCompress())
        {
            throw std::logic_error("pointer compress is not enabled!");
        }

        if (!p)
        {
            return 0;
        }

        return static_cast<uint32_t>((reinterpret_cast<uintptr_t>(p) - reinterpret_cast<uintptr_t>(rawHeap.block)) >>
                                     3);
    }

    inline auto decompress(uint32_t p) -> void *
    {
        if (!enablePtrCompress())
        {
            throw std::logic_error("pointer compress is not enabled!");
        }

        if (!p)
        {
            return nullptr;
        }

        return reinterpret_cast<void *>((static_cast<uintptr_t>(p) << 3) + reinterpret_cast<uintptr_t>(rawHeap.block));
    }

    auto valid(void *ptr) -> bool
    {
        return rawHeap.vaild(ptr);
    }

  private:
    log::OMLogger logger;

  public:
    mem::OMHeap rawHeap;
    OMElysiaHeapBlock *emptyBlocks;
    std::mutex blockMutex;

    uint64_t maxSize;

    float expandFactor;
};
} // namespace openminecraft::vm::elysia

#endif
