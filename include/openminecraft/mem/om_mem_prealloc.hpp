#ifndef OM_MEM_PREALLOC_HPP
#define OM_MEM_PREALLOC_HPP

#include "openminecraft/log/om_log_common.hpp"
#include <cstdint>
namespace openminecraft::mem
{
class OMHeap
{
  public:
    OMHeap(uint64_t minSize, uint64_t maxSize);
    ~OMHeap();

    void init();
    void expand(void *target);
    void shrink(void *target);
    void activateExecutable(void *p, uint64_t length);

    uint64_t currentSizeAllocated();

    bool vaild(void *p)
    {
        return p >= block && p < heapTop;
    }

    void *block;
    void *heapTop{};

  private:
    log::OMLogger logger;
    uint64_t minSize;
    uint64_t maxSize;

    void activate(void *p, uint64_t length);
    void deactivate(void *p, uint64_t length);
};
} // namespace openminecraft::mem

#endif