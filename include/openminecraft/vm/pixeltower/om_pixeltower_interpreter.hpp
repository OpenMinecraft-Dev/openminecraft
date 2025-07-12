#ifndef OM_PIXELTOWER_INTERPRETER_HPP
#define OM_PIXELTOWER_INTERPRETER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/heap/om_heap_tree.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_linker.hpp"
#include <any>
#include <memory>
#include <stack>
#include <vector>
namespace openminecraft::vm::pixeltower
{
struct OMFrameMetadata
{
    std::string clazz;
    std::string method;
    std::string sig;
    bool isNative;
    uint64_t offset;
    uint64_t allowedStackDepth;
    std::shared_ptr<std::vector<std::any *>> locals;
    std::vector<void *> allocatedObjects;
    OMMethodInfo methodInfo;
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

    void executeBytecode(std::shared_ptr<OMClass> f, classfile::OMClassAttrCode *codeWrap,
                         std::shared_ptr<OMFrameMetadata> frame);

    void operand_nop(uint64_t &offset);
    void operand_iconst(uint64_t &offset, int data);
    void operand_istore(uint64_t &offset, int data, std::shared_ptr<OMFrameMetadata> frame);
    void operand_new(uint64_t &offset, std::string type);
    void operand_dup(uint64_t &offset);
    void operand_pop(uint64_t &offset);
    void operand_invokespecial(uint64_t &offset, std::string clazz, std::string func, std::string desc);
    void operand_invokevirtual(uint64_t &offset, std::string clazz, std::string func, std::string desc);
    void operand_invokestatic(uint64_t &offset, std::string clazz, std::string func, std::string desc);
    void operand_return();
    std::stack<std::any, std::list<std::any>> stack;

  private:
    void debugStack();
    void logAnyData(int idx, std::any data);
    heap::OMHeapTree memoryTree;
    log::OMLogger logger;

    OMClassLoader loader;
    OMLinker linker;
};
}; // namespace openminecraft::vm::pixeltower

#endif