#ifndef OM_BYTECODE_CHECKER_HPP
#define OM_BYTECODE_CHECKER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include <any>
#include <memory>
#include <stack>
namespace openminecraft::vm::bytecode
{
class OMBytecodeChecker
{
  public:
    OMBytecodeChecker(std::shared_ptr<classfile::OMClassFile> cls);
    void detail();
    util::OMResult<std::any, err::OMValidationError> constantCheck();

  private:
    std::string funcName(classfile::OMClassMethodInfo info);
    std::shared_ptr<classfile::OMClassFile> cls;
    std::unique_ptr<log::OMLogger> logger;
    std::unique_ptr<log::OMLogger> loggerSub;
};
}; // namespace openminecraft::vm::bytecode

#endif
