#ifndef OM_ELYSIA_HEAP
#define OM_ELYSIA_HEAP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_prealloc.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>

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
    OMElysiaHeap(const char *name, uint64_t maxSize);
    ~OMElysiaHeap();

    void *allocate(uint64_t length);
    void deallocate(void *ptr, uint64_t length);
    void mergeBlocks();
    void iterBlocks(std::function<void(OMElysiaHeapBlock *)>);
    void *base()
    {
        return rawHeap.block;
    }
    uint64_t align(uint64_t size)
    {
        return (size % 8) ? (size + (8 - size % 8)) : size;
    }

    template <typename T> T *allocate()
    {
        return reinterpret_cast<T *>(allocate(sizeof(T)));
    }
    template <typename T> void deallocate(T *ptr)
    {
        deallocate(ptr, sizeof(T));
    }

  private:
    log::OMLogger logger;

    mem::OMHeap rawHeap;

    OMElysiaHeapBlock *emptyBlocks;
    std::mutex blockMutex;
};
} // namespace openminecraft::vm::elysia

#endif
