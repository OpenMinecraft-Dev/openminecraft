#ifndef OM_PIXELTOWER_VALIDATOR_HPP
#define OM_PIXELTOWER_VALIDATOR_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include <cstdint>
#include <map>
#include <memory>
#include <stack>
#include <vector>
namespace openminecraft::vm::pixeltower::v3
{
struct OMContext
{
    std::vector<int> locals;
    std::stack<int> stack;
};
class OMValidator
{
  public:
    OMValidator();
    ~OMValidator() = default;

    void validate(std::shared_ptr<classfile::OMClassFile> file, std::string name);

  private:
    void validateConstantPool(std::shared_ptr<classfile::OMClassFile> file, std::string name);
    void checkRecursively(std::shared_ptr<classfile::OMClassFile> file, uint16_t id, std::string name,
                          classfile::OMClassConstantType type);

    void checkMethod(std::shared_ptr<classfile::OMClassFile> file, std::shared_ptr<classfile::OMClassMethodInfo> method,
                     std::string name, std::map<int, bool> &checked, OMContext *context = nullptr, int offset = 0);
    std::string fetchContent(int flags);

    void safeStackPush(std::stack<int> &stack, classfile::OMClassAttrCode *code, std::string pos, int i);
    int safeStackPop(std::stack<int> &stack, classfile::OMClassAttrCode *code, std::string pos, int i);
    void safeStackCheck(std::stack<int> &stack, classfile::OMClassAttrCode *code, std::string pos, int i);
    void safeLocalSet(std::vector<int> &local, classfile::OMClassAttrCode *code, std::string pos, int index, int i);
    void safeLocalGet(std::vector<int> &local, classfile::OMClassAttrCode *code, std::string pos, int index, int i);
    void safeArgFetch(std::stack<int> &stack, classfile::OMClassAttrCode *code, std::string pos, std::string desc);
    void safeReturnFetch(std::stack<int> &stack, classfile::OMClassAttrCode *code, std::string pos, std::string desc);
    int toFlag(bytecode::descriptor::OMTypeDesc name);

    log::OMLogger logger;
};
} // namespace openminecraft::vm::pixeltower::v3

#endif