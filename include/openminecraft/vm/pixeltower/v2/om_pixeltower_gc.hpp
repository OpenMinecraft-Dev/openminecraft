#ifndef OM_PIXELTOWER_GC_HPP
#define OM_PIXELTOWER_GC_HPP

#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
namespace openminecraft::vm::pixeltower::v2
{
class OMGarbageCollector
{
  public:
    OMGarbageCollector(v0::OMPixelTowerHeap *heap, v0::OMPixelTower *tower) : heap(heap), tower(tower)
    {
    }
    virtual ~OMGarbageCollector() = default;

    virtual void signUnreachable() = 0;
    virtual void freeObjects() = 0;

  protected:
    v0::OMPixelTowerHeap *heap;
    v0::OMPixelTower *tower;
};
} // namespace openminecraft::vm::pixeltower::v2

#endif