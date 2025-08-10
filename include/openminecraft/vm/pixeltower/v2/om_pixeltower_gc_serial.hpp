#ifndef OM_PIXELTOWER_GC_SERIAL_HPP
#define OM_PIXELTOWER_GC_SERIAL_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/pixeltower/v2/om_pixeltower_gc.hpp"
namespace openminecraft::vm::pixeltower::v2
{
class OMGarbageCollectorSerial : public OMGarbageCollector
{
  public:
    OMGarbageCollectorSerial(v0::OMPixelTowerHeap *heap, v0::OMPixelTower *tower);

    virtual void signUnreachable() override;
    virtual void freeObjects() override;

  private:
    log::OMLogger logger;
    bool existsInStack(void *p);
    void markSub(void *root);
};
} // namespace openminecraft::vm::pixeltower::v2

#endif