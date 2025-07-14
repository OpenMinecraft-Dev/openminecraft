#ifndef OM_PIXELTOWER_INTERPRETER_HPP
#define OM_PIXELTOWER_INTERPRETER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_frame_structdef.hpp"
#include <any>
#include <memory>
#include <stack>
namespace openminecraft::vm::pixeltower::runtime
{
class OMInterpreter
{
  public:
    OMInterpreter(OMPixelTower &tower);
    ~OMInterpreter();

    util::OMResult<std::any, err::OMValidationError> execute(std::string clazz, std::string method, std::string sig)
    {
        auto cls = tower.fetchClass(clazz);
        if (cls.type == util::Err)
        {
            return util::OMResult<std::any, err::OMValidationError>::err(cls.unwrap_err());
        }
        return execute(cls.unwrap(), method, sig);
    }
    util::OMResult<std::any, err::OMValidationError> execute(std::shared_ptr<OMClass> clazz, std::string method,
                                                             std::string sig);
    util::OMResult<std::any, err::OMValidationError> execute(std::shared_ptr<OMClass> clazz,
                                                             std::shared_ptr<OMMethodInfo> mi);

    std::stack<std::any> stack;

    void operand_invokestatic(std::shared_ptr<OMFrameMetadata> frame);

  private:
    OMPixelTower &tower;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::pixeltower::runtime

#endif