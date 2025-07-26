#ifndef OM_PIXELTOWER_INTERPRETER_HPP
#define OM_PIXELTOWER_INTERPRETER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
namespace openminecraft::vm::pixeltower::v0
{
class OMInterpreter
{
  public:
    OMInterpreter(OMPixelTowerHeap *heap);
    ~OMInterpreter();

    bool execute();

  private:
    log::OMLogger logger;
    OMPixelTowerHeap *heap;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif