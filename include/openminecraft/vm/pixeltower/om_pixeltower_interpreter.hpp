#ifndef OM_PIXELTOWER_INTERPRETER_HPP
#define OM_PIXELTOWER_INTERPRETER_HPP

#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower.hpp"
#include <any>
#include <stack>
namespace openminecraft::vm::pixeltower::runtime
{
class OMInterpreter
{
  public:
    OMInterpreter(OMPixelTower &tower);
    ~OMInterpreter();

    util::OMResult<std::any, err::OMValidationError> execute();

    std::stack<std::any> stack;

  private:
    OMPixelTower &tower;
};
} // namespace openminecraft::vm::pixeltower::runtime

#endif