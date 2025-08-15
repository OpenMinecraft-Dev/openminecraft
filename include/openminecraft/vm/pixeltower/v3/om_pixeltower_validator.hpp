#ifndef OM_PIXELTOWER_VALIDATOR_HPP
#define OM_PIXELTOWER_VALIDATOR_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include <memory>
namespace openminecraft::vm::pixeltower::v3
{
class OMValidator
{
  public:
    OMValidator();
    ~OMValidator() = default;

    void validate(std::shared_ptr<classfile::OMClassFile> file, std::string name);

  private:
    void validateConstantPool(std::shared_ptr<classfile::OMClassFile> file, std::string name);
    void checkRecursively(std::shared_ptr<classfile::OMClassFile> file, std::shared_ptr<classfile::OMClassConstant> c,
                          uint16_t id, std::string name, classfile::OMClassConstantType type);

    log::OMLogger logger;
};
} // namespace openminecraft::vm::pixeltower::v3

#endif