#ifndef OM_PIXELTOWER_INTERPRETER_HPP
#define OM_PIXELTOWER_INTERPRETER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/heap/om_heap_tree.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_type.hpp"
#include <any>
#include <memory>
#include <stack>
#include <unordered_map>
#include <vector>
namespace openminecraft::vm::pixeltower
{
struct OMFrameMetadata
{
    std::string clazz;
    std::string method;
    std::string sig;
    uint64_t offset;
    uint64_t allowedStackDepth;
    std::shared_ptr<std::vector<std::any *>> locals;
};

template <typename T> struct OMArray
{
    uint32_t length;
    T *data;
};

struct OMLocalVariablePlaceholder
{
};

class OMInterpreter
{
  public:
    OMInterpreter();
    ~OMInterpreter();

    void loadClass(std::shared_ptr<vm::classfile::OMClassFile> f);
    void execute(std::string clazz, std::string func, std::string desc, bool isStatic);

    void executeBytecode(std::shared_ptr<vm::classfile::OMClassFile> f, classfile::OMClassAttrCode *codeWrap,
                         std::shared_ptr<OMFrameMetadata> frame);
    bool findAndExecuteBytecode(std::string clazz, std::string func, std::string desc, bool isStatic);

    void operand_nop(uint64_t &offset);
    void operand_new(uint64_t &offset, std::string type);
    void operand_dup(uint64_t &offset);
    void operand_invokespecial(uint64_t &offset, std::string clazz, std::string func, std::string desc);
    void operand_invokevirtual(uint64_t &offset, std::string clazz, std::string func, std::string desc);
    void operand_invokestatic(uint64_t &offset, std::string clazz, std::string func, std::string desc);
    std::stack<std::any, std::list<std::any>> stack;

  private:
    void debugStack();
    void logAnyData(int idx, std::any data);
    heap::OMHeapTree memoryTree;
    log::OMLogger logger;

    OMClassLoader loader;
};
}; // namespace openminecraft::vm::pixeltower

#endif