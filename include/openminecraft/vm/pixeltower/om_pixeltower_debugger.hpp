#ifndef OM_PIXELTOWER_DEBUGGER_HPP
#define OM_PIXELTOWER_DEBUGGER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include <any>
#include <memory>
namespace openminecraft::vm::pixeltower::v1
{
class OMDebugger
{
  public:
    OMDebugger(std::shared_ptr<runtime::OMInterpreter> interpreter);
    void printStack();

  private:
    std::string serializeAny(std::any data, int objDepth = 0);
    static std::string removeAnyPrefix(std::string s);

    std::shared_ptr<runtime::OMInterpreter> interpreter;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::pixeltower::v1

#endif