#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_ansi.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/heap/om_heap_tree.hpp"
#include <any>
#include <memory>
#include <typeindex>
#include <vector>

using namespace openminecraft::vm::classfile;
using namespace openminecraft::binary::hash;
using namespace openminecraft::log::ansi;

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
            operand_nop(frame->offset);
            break;
        case op_new: {
            operand_new(frame->offset);
            break;
        }
        case op_dup: {
            operand_dup(frame->offset);
            break;
        }
        default:
            logger.warn("unknown instruction!");
            goto execute_fail;
        }
    }

execute_fail:
    stack.push(0.11f);
    uint64_t idx = 0;
    while (!stack.empty())
    {
        auto target = std::type_index(stack.top().type());
        if (target == std::type_index(typeid(void *)))
        {
            logger.info("{3}[{0}] {2}{1:#018x}{4} at heap tree", idx,
                        (uint64_t)(void *)std::any_cast<void *>(stack.top()), OMLogAnsiYellowLight, OMLogAnsiBlueLight,
                        OMLogAnsiReset);
        }
        else if (target == std::type_index(typeid(std::shared_ptr<OMFrameMetadata>)))
        {
            auto fmd = std::any_cast<std::shared_ptr<OMFrameMetadata>>(stack.top());
            logger.info("{6}[{0}]{7} frame metadata {8}{1}.{2}{7}{9}{3}{7} + {5}{4}{7}", idx, fmd->clazz, fmd->method,
                        fmd->sig, fmd->offset, OMLogAnsiYellowLight, OMLogAnsiBlueLight, OMLogAnsiReset,
                        OMLogAnsiCyanLight, OMLogAnsiBlackLight);
        }
        else if (target == std::type_index(typeid(float)))
        {
            logger.info("{3}[{0}] {2}{1}f{4}", idx, std::any_cast<float>(stack.top()), OMLogAnsiGreenLight,
                        OMLogAnsiBlueLight, OMLogAnsiReset);
        }

        stack.pop();
        idx++;
    }
    return;
}
void OMInterpreter::operand_nop(uint64_t &offset)
{
    offset++;
}
void OMInterpreter::operand_new(uint64_t &offset)
{
    auto id = memoryTree.allocateId();
    stack.push((void *)id);
    memoryTree.allocate(id, 0);
    memoryTree.attach(heap::heapRoot, id);
    offset += 3;
}
void OMInterpreter::operand_dup(uint64_t &offset)
{
    stack.push(stack.top());
    offset++;
}
} // namespace openminecraft::vm::pixeltower