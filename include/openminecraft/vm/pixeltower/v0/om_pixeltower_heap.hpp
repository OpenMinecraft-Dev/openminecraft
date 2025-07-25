#ifndef OM_PIXELTOWER_HEAP_HPP
#define OM_PIXELTOWER_HEAP_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_prealloc.hpp"
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

  private:
    mem::OMHeap *heap;
    std::mutex emptyBlockMutex;
    std::vector<OMHeapEmptyBlock> emptyBlocks;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif