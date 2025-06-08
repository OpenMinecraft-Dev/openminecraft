#ifndef OM_BYTECODE_CHECKER_HPP
#define OM_BYTECODE_CHECKER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/om_class_file.hpp"
#include <memory>
namespace openminecraft::vm::bytecode
{
class OMBytecodeChecker
{
  public:
    OMBytecodeChecker(std::shared_ptr<classfile::OMClassFile> cls);
    void check();

  private:
    std::shared_ptr<classfile::OMClassFile> cls;
    std::unique_ptr<log::OMLogger> logger;
};
}; // namespace openminecraft::vm::bytecode

#endif