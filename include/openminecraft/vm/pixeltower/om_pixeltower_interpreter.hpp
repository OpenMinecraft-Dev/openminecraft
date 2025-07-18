#ifndef OM_PIXELTOWER_INTERPRETER_HPP
#define OM_PIXELTOWER_INTERPRETER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_array_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_frame_structdef.hpp"
#include <any>
#include <memory>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>
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
    util::OMResult<std::any, err::OMValidationError> executeDynamic(std::string clazz, std::string method,
                                                                    std::string sig);
    util::OMResult<std::any, err::OMValidationError> execute(std::shared_ptr<OMClass> clazz,
                                                             std::shared_ptr<OMMethodInfo> mi);

    std::unordered_map<std::thread::id, std::stack<std::any>> stack;

    util::OMResult<std::any, err::OMValidationError> operand_invokeany(std::shared_ptr<OMFrameMetadata> frame);
    void operand_dup(std::shared_ptr<OMFrameMetadata> frame);
    util::OMResult<std::any, err::OMValidationError> operand_return(std::shared_ptr<OMFrameMetadata> frame);
    util::OMResult<std::any, err::OMValidationError> operand_ireturn(std::shared_ptr<OMFrameMetadata> frame);
    void operand_new(std::shared_ptr<OMFrameMetadata> frame);
    void operand_aload_n(std::shared_ptr<OMFrameMetadata> frame);
    util::OMResult<std::any, err::OMValidationError> operand_if_acmpne(std::shared_ptr<OMFrameMetadata> frame);
    void operand_iconst_n(std::shared_ptr<OMFrameMetadata> frame);
    void operand_lconst_n(std::shared_ptr<OMFrameMetadata> frame);
    void operand_fconst_n(std::shared_ptr<OMFrameMetadata> frame);
    void operand_dconst_n(std::shared_ptr<OMFrameMetadata> frame);
    void operand_nop(std::shared_ptr<OMFrameMetadata> frame);
    void operand_aconst_null(std::shared_ptr<OMFrameMetadata> frame);
    void operand_pop(std::shared_ptr<OMFrameMetadata> frame);
    void operand_astore_n(std::shared_ptr<OMFrameMetadata> frame);
    void operand_istore_n(std::shared_ptr<OMFrameMetadata> frame);
    util::OMResult<std::any, err::OMValidationError> operand_invokeinterface(std::shared_ptr<OMFrameMetadata> frame);
    util::OMResult<std::any, err::OMValidationError> operand_putstatic(std::shared_ptr<OMFrameMetadata> frame);
    util::OMResult<std::any, err::OMValidationError> operand_putfield(std::shared_ptr<OMFrameMetadata> frame);

    void *newString(std::string data);

  private:
    util::OMResult<std::any, err::OMValidationError> checkType(std::shared_ptr<OMFrameMetadata> frame, std::any data,
                                                               std::string desc);
    std::string fetchName(OMArrayType type);
    void writeStackTop(void *target, std::string desc);
    std::string fetchCurrentPosition(std::shared_ptr<OMFrameMetadata> frame);
    util::OMResult<std::any, err::OMValidationError> popFrame(std::shared_ptr<OMFrameMetadata> frame);

    OMPixelTower &tower;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::pixeltower::runtime

#endif