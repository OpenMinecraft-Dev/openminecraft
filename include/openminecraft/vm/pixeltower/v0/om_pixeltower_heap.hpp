#ifndef OM_PIXELTOWER_HEAP_HPP
#define OM_PIXELTOWER_HEAP_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_prealloc.hpp"
#include <cassert>
#include <cstdint>
#include <mutex>
#include <vector>
namespace openminecraft::vm::pixeltower::v0
{
struct OMHeapEmptyBlock
{
    void *ptr;
    uint64_t length;
};

class OMPixelTowerHeap
{
  public:
    OMPixelTowerHeap(uint64_t minSize, uint64_t maxSize);
    ~OMPixelTowerHeap();

    void *allocate(uint64_t length);
    void deallocate(void *ptr, uint64_t length);
    void debug();
    void merge();

    inline void *heapBase()
    {
        return heap->block;
    }

    inline void *heapTop()
    {
        return heap->heapTop;
    }

    inline bool ptrCompEnabled()
    {
        return maxSize < 32ull * 1024 * 1024 * 1024 && (sizeof(void *) == 8);
    }
    inline uint64_t ptrSize()
    {
        return ptrCompEnabled() ? 4 : sizeof(void *);
    }

    inline uint32_t compressPtr(void *p)
    {
        assert(sizeof(void *) == 8);
        return ((size_t)p - (size_t)heap->block) >> 3;
    }
    inline void *decompressPtr(uint32_t p)
    {
        assert(sizeof(void *) == 8);
        return (void *)((size_t)(p << 3) + (size_t)heap->block);
    }

    inline bool inside(void *p)
    {
        return heap->vaild(p);
    }

    inline void *nextPtr(void *p, uint64_t length)
    {
        auto r = ((uint8_t *)p) + length;
        for (auto &p : emptyBlocks)
        {
            if (r == p.ptr)
            {
                auto rpp = reinterpret_cast<void *>(((uint8_t *)r) + p.length);
                if (inside(rpp))
                {
                    return rpp;
                }
                else
                {
                    return nullptr;
                }
            }
        }
        return r;
    }

    inline double usage()
    {
        auto total = heap->currentSizeAllocated();
        uint64_t t = 0;
        for (auto &p : emptyBlocks)
        {
            t += p.length;
        }

        return 1.0 - ((double)t / (double)total);
    }

  private:
    uint64_t maxSize;
    mem::OMHeap *heap;
    std::mutex emptyBlockMutex;
    std::vector<OMHeapEmptyBlock> emptyBlocks;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif