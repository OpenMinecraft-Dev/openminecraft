#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/heap/om_heap_tree.hpp"
#include <memory>
#include <vector>

using namespace openminecraft::vm::classfile;

namespace openminecraft::vm::pixeltower
{
OMInterpreter::OMInterpreter() : logger("OMInterpreter", this)
{
}
OMInterpreter::~OMInterpreter()
{
}
void OMInterpreter::interpret(std::shared_ptr<vm::classfile::OMClassFile> f, std::string func)
{
    OMClassAttrCode *codeWrap;
    for (auto method : f->methods)
    {
        auto name = f->mapping[method->nameIndex]->to<OMClassConstantUtf8>()->data;
        if (func == name)
        {
            for (auto attr : method->attrs)
            {
                if (attr->type() == classfile::Code)
                {
                    codeWrap = attr->to<OMClassAttrCode>();
                    goto execute;
                }
            }
        }
    }
    return;
execute:
    auto cls =
        f->mapping[f->mapping[f->thisClass]->to<OMClassConstantClass>()->nameIndex]->to<OMClassConstantUtf8>()->data;
    auto frame = std::make_shared<OMFrameMetadata>(OMFrameMetadata{cls, func, "([java/lang/String)V", 0});
    stack.push(frame);

    while (frame->offset < codeWrap->codeLength)
    {
        switch (codeWrap->code[frame->offset])
        {
        case op_nop:
            operand_nop();
            frame->offset++;
            break;
        case op_new: {
            operand_new();
            frame->offset += 2;
            break;
        }
        case op_dup: {
            operand_dup();
            frame->offset++;
            break;
        }
        default:
            goto execute_fail;
        }
    }

execute_fail:
    while (!stack.empty())
    {
        logger.info("{}", stack.top().type().name());
        stack.pop();
    }
    mem::castorice::printres();
    return;
}
void OMInterpreter::operand_nop()
{
}
void OMInterpreter::operand_new()
{
    auto id = memoryTree.allocateId();
    stack.push((void *)id);
    memoryTree.allocate(id, 0);
    memoryTree.attach(heap::heapRoot, id);
}
void OMInterpreter::operand_dup()
{
    stack.push(stack.top());
}
} // namespace openminecraft::vm::pixeltower