#ifndef OM_PIXELTOWER_INTERPRETER_HPP
#define OM_PIXELTOWER_INTERPRETER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
#include "openminecraft/vm/pixeltower/v1/om_pixeltower_debugger.hpp"
#include <vector>
namespace openminecraft::vm::pixeltower::v0
{
class OMPixelTower;
#define EXEC_RETURN 0
#define EXEC_FAIL 1
class OMInterpreter
{
  public:
    OMInterpreter(OMPixelTowerHeap *heap, OMPixelTower *tower);
    ~OMInterpreter();

    jint execute();
    void call(OMMethod *met, uint8_t *retAddr);
    OMPixelTower *tower;

  private:
    void invokeNative(OMMethod *m, std::vector<void *> &args);
    void callDynamic(OMMethod *met, uint8_t *retAddr);
    void popLastFrame();
    inline void loop()
    {
        switch (jint result = execute())
        {
        case EXEC_RETURN:
            break;
        default:
            logger.error("function execution failed with code {}", result);
            throw result;
        }
    }

    log::OMLogger logger;
    OMPixelTowerHeap *heap;
    v1::OMDebugger debugger;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif
