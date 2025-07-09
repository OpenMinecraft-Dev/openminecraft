#ifndef OM_PIXELTOWER_INTERPRETER_HPP
#define OM_PIXELTOWER_INTERPRETER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/heap/om_heap_tree.hpp"
#include <any>
#include <memory>
#include <stack>
namespace openminecraft::vm::pixeltower
{
struct OMFrameMetadata
{
    std::string clazz;
    std::string method;
    std::string sig;
    uint64_t offset;
};

class OMInterpreter
{
  public:
    OMInterpreter();
    ~OMInterpreter();

    void interpret(std::shared_ptr<vm::classfile::OMClassFile> f, std::string func);

    void operand_nop(uint64_t &offset);
    void operand_new(uint64_t &offset);
    void operand_dup(uint64_t &offset);

  private:
    std::stack<std::any> stack;
    heap::OMHeapTree memoryTree;
    log::OMLogger logger;
};
}; // namespace openminecraft::vm::pixeltower

#endif