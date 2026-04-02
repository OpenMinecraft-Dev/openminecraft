#ifndef OM_ELYSIA_HEAP
#define OM_ELYSIA_HEAP

#include "openminecraft/mem/om_mem_prealloc.hpp"

namespace openminecraft::vm::elysia
{
struct OMElysiaHeapBlock
{
    void *block;
    uint64_t size;
    OMElysiaHeapBlock *next;
};

class OMElysiaHeap
{
  public:
    OMElysiaHeap(std::string name, uint64_t maxSize);
    ~OMElysiaHeap();

  private:
    mem::OMHeap rawHeap;

    OMElysiaHeapBlock *emptyBlocks;
};
} // namespace openminecraft::vm::elysia

#endif
