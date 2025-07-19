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

    void execute(std::string clazz, std::string method, std::string sig);
    void execute(std::shared_ptr<OMClass> clazz, std::string method, std::string sig);
    void executeDynamic(std::string clazz, std::string method, std::string sig, std::shared_ptr<OMFrameMetadata> frame);
    void execute(std::shared_ptr<OMClass> clazz, std::shared_ptr<OMMethodInfo> mi);

    std::unordered_map<std::thread::id, std::stack<std::any>> stack;

    void operand_invokeany(std::shared_ptr<OMFrameMetadata> frame);
    void operand_dup(std::shared_ptr<OMFrameMetadata> frame);
    void operand_return(std::shared_ptr<OMFrameMetadata> frame);
    void operand_ireturn(std::shared_ptr<OMFrameMetadata> frame);
    void operand_new(std::shared_ptr<OMFrameMetadata> frame);
    void operand_aload_n(std::shared_ptr<OMFrameMetadata> frame);
    void operand_if_acmpne(std::shared_ptr<OMFrameMetadata> frame);
    void operand_iconst_n(std::shared_ptr<OMFrameMetadata> frame);
    void operand_lconst_n(std::shared_ptr<OMFrameMetadata> frame);
    void operand_fconst_n(std::shared_ptr<OMFrameMetadata> frame);
    void operand_dconst_n(std::shared_ptr<OMFrameMetadata> frame);
    void operand_nop(std::shared_ptr<OMFrameMetadata> frame);
    void operand_aconst_null(std::shared_ptr<OMFrameMetadata> frame);
    void operand_pop(std::shared_ptr<OMFrameMetadata> frame);
    void operand_astore_n(std::shared_ptr<OMFrameMetadata> frame);
    void operand_istore_n(std::shared_ptr<OMFrameMetadata> frame);
    void operand_invokeinterface(std::shared_ptr<OMFrameMetadata> frame);
    void operand_putstatic(std::shared_ptr<OMFrameMetadata> frame);
    void operand_putfield(std::shared_ptr<OMFrameMetadata> frame);
    void operand_getstatic(std::shared_ptr<OMFrameMetadata> frame);
    void operand_getfield(std::shared_ptr<OMFrameMetadata> frame);
    void operand_iload_n(std::shared_ptr<OMFrameMetadata> frame);
    void operand_bipush(std::shared_ptr<OMFrameMetadata> frame);
    void operand_invokevirtual(std::shared_ptr<OMFrameMetadata> frame);
    void operand_ldc(std::shared_ptr<OMFrameMetadata> frame);
    void operand_ldc_w(std::shared_ptr<OMFrameMetadata> frame);
    void operand_iadd(std::shared_ptr<OMFrameMetadata> frame);
    void operand_i2b(std::shared_ptr<OMFrameMetadata> frame);
    void operand_i2s(std::shared_ptr<OMFrameMetadata> frame);

    void *newString(std::string data);

  private:
    void checkType(std::shared_ptr<OMFrameMetadata> frame, std::any data, std::string desc);
    std::string fetchName(OMArrayType type);
    void writeStackTop(void *target, std::string desc, std::shared_ptr<OMFrameMetadata> frame);
    void fetchToStackTop(void *target, std::string desc, std::shared_ptr<OMFrameMetadata> frame);
    std::string fetchCurrentPosition(std::shared_ptr<OMFrameMetadata> frame);
    void constantInternal(std::shared_ptr<OMFrameMetadata> frame, uint16_t id);
    void popFrame(std::shared_ptr<OMFrameMetadata> frame);

    OMPixelTower &tower;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::pixeltower::runtime

#endif