#ifndef OM_PIXELTOWER_INTERPRETER_HPP
#define OM_PIXELTOWER_INTERPRETER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
#include "openminecraft/vm/pixeltower/v1/om_pixeltower_debugger.hpp"
namespace openminecraft::vm::pixeltower::v0
{
class OMPixelTower;
class OMInterpreter
{
  public:
    OMInterpreter(OMPixelTowerHeap *heap, OMPixelTower *tower);
    ~OMInterpreter();

    bool execute();

  private:
    void call(OMMethod *met);
    void callDynamic(OMMethod *met);
    void popLastFrame();

    log::OMLogger logger;
    OMPixelTower *tower;
    OMPixelTowerHeap *heap;
    v1::OMDebugger debugger;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif
