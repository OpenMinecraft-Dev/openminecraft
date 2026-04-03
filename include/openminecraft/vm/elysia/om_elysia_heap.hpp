#ifndef OM_ELYSIA_HEAP
#define OM_ELYSIA_HEAP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_prealloc.hpp"
#include <mutex>

namespace openminecraft::vm::elysia
{
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

  private:
    log::OMLogger logger;

    mem::OMHeap rawHeap;

    OMElysiaHeapBlock *emptyBlocks;
    std::mutex blockMutex;
};
} // namespace openminecraft::vm::elysia

#endif
