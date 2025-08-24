#ifndef OM_PIXELTOWER_INTERPRETER_HPP
#define OM_PIXELTOWER_INTERPRETER_HPP

#include "om_pixeltower.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/err/om_runtime_error.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_threads.hpp"
#include "openminecraft/vm/pixeltower/v1/om_pixeltower_debugger.hpp"
#include <vector>
namespace openminecraft::vm::pixeltower::v0
{
class OMPixelTower;
constexpr int EXEC_RETURN = 0;
constexpr int EXEC_FAIL = 1;
class OMInterpreter
{
  public:
    OMInterpreter(OMPixelTowerHeap *heap, OMPixelTower *tower);
    ~OMInterpreter();

    jint execute();
    void call(OMMethod *met, uint8_t *retAddr);
    void checkNotNull(const void *p) const;
    OMPixelTower *tower;
    v1::OMDebugger debugger;
    uint64_t operands = 0;
    std::vector<void *> bootArgs;

  private:
    void validateArgs();
    bool checkCompat(OMKlass *src, OMKlass *target);

    void invokeNative(OMMethod *m, std::vector<void *> &args);
    void callDynamic(OMMethod *met, uint8_t *retAddr);
    void popLastFrame();
    void loop()
    {
        jint result = execute();
        if (result != EXEC_RETURN)
        {
            logger.error("function execution failed with code {}", result);
            throw result;
        }
    }

    static void throwTypeCheckError(std::string r)
    {
        throw err::OMValidationError{err::Instructions, r, currentPosition()};
    }

    log::OMLogger logger;
    OMPixelTowerHeap *heap;
};
} // namespace openminecraft::vm::pixeltower::v0

#endif
