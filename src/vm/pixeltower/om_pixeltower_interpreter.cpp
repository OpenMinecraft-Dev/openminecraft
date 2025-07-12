#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_ansi.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/heap/om_heap_tree.hpp"
#include "openminecraft/vm/pixeltower/stdlib/om_stdlib_object.hpp"
#include <algorithm>
#include <any>
#include <memory>
#include <stack>
#include <stdexcept>
#include <typeindex>
#include <vector>

using namespace openminecraft::vm::classfile;
using namespace openminecraft::binary::hash;
using namespace openminecraft::log::ansi;

namespace openminecraft::vm::pixeltower
{
OMInterpreter::OMInterpreter() : logger("pixeltower/OMInterpreter", this), linker(this->loader)
{
    loader.loadBasicClasses();
}
OMInterpreter::~OMInterpreter()
{
}
void OMInterpreter::loadClass(std::shared_ptr<vm::classfile::OMClassFile> f)
{
    loader.loadClass(f);
}
void OMInterpreter::executeBytecode(std::shared_ptr<OMClass> f, OMClassAttrCode *codeWrap,
                                    std::shared_ptr<OMFrameMetadata> frame)
{
    auto mp = *f->mapping;
    while (frame->offset < codeWrap->codeLength)
    {
        switch (codeWrap->code[frame->offset])
        {
        case op_nop: {
            operand_nop(frame->offset);
            break;
        }
        case op_iconst_i(-1):
        case op_iconst_i(0):
        case op_iconst_i(1):
        case op_iconst_i(2):
        case op_iconst_i(3):
        case op_iconst_i(4):
        case op_iconst_i(5): {
            operand_iconst(frame->offset, codeWrap->code[frame->offset] - op_iconst_i(0));
            break;
        }
        case op_istore_n(0):
        case op_istore_n(1):
        case op_istore_n(2):
        case op_istore_n(3): {
            operand_istore(frame->offset, codeWrap->code[frame->offset] - op_istore_n(0), frame);
            break;
        }
        case op_pop: {
            operand_pop(frame->offset);
            break;
        }
        case op_new: {
            auto id = binary::be16ToNative(*(uint16_t *)(codeWrap->code.data() + frame->offset + 1));
            auto type = mp[mp[id]->to<OMClassConstantClass>()->nameIndex]->to<OMClassConstantUtf8>()->data;
            operand_new(frame->offset, type);
            frame->allocatedObjects.push_back(std::any_cast<void *>(stack.top()));
            break;
        }
        case op_dup: {
            operand_dup(frame->offset);
            break;
        }
        case op_invokespecial: {
            auto id = binary::be16ToNative(*(uint16_t *)(codeWrap->code.data() + frame->offset + 1));
            auto temp1 = mp[id]->to<OMClassConstantMethodRef>();
            auto cls =
                mp[mp[temp1->classIndex]->to<OMClassConstantClass>()->nameIndex]->to<OMClassConstantUtf8>()->data;
            auto temp2 = mp[temp1->nameAndTypeIndex]->to<OMClassConstantNameAndType>();
            auto name = mp[temp2->nameIndex]->to<OMClassConstantUtf8>()->data;
            auto desc = mp[temp2->descIndex]->to<OMClassConstantUtf8>()->data;
            operand_invokespecial(frame->offset, cls, name, desc);
            break;
        }
        case op_invokevirtual: {
            auto id = binary::be16ToNative(*(uint16_t *)(codeWrap->code.data() + frame->offset + 1));
            auto temp1 = mp[id]->to<OMClassConstantMethodRef>();
            auto cls =
                mp[mp[temp1->classIndex]->to<OMClassConstantClass>()->nameIndex]->to<OMClassConstantUtf8>()->data;
            auto temp2 = mp[temp1->nameAndTypeIndex]->to<OMClassConstantNameAndType>();
            auto name = mp[temp2->nameIndex]->to<OMClassConstantUtf8>()->data;
            auto desc = mp[temp2->descIndex]->to<OMClassConstantUtf8>()->data;
            operand_invokevirtual(frame->offset, cls, name, desc);
            break;
        }
        case op_invokestatic: {
            auto id = binary::be16ToNative(*(uint16_t *)(codeWrap->code.data() + frame->offset + 1));
            auto temp1 = mp[id]->to<OMClassConstantMethodRef>();
            auto cls =
                mp[mp[temp1->classIndex]->to<OMClassConstantClass>()->nameIndex]->to<OMClassConstantUtf8>()->data;
            auto temp2 = mp[temp1->nameAndTypeIndex]->to<OMClassConstantNameAndType>();
            auto name = mp[temp2->nameIndex]->to<OMClassConstantUtf8>()->data;
            auto desc = mp[temp2->descIndex]->to<OMClassConstantUtf8>()->data;
            operand_invokestatic(frame->offset, cls, name, desc);
            break;
        }
        case op_return: {
            operand_return();
            return;
        }
        default:
            logger.warn("unknown instruction!");
            operand_return();
            return;
        }
    }
}

void OMInterpreter::execute(std::string clazz, std::string func, std::string desc, bool isStatic)
{
    linker.callMethod(this, clazz, func, desc, isStatic, stack);

    return;
}
void OMInterpreter::debugStack()
{
    std::stack<std::any, std::list<std::any>> debugs(stack);
    int idx = 0;
    while (!debugs.empty())
    {
        logAnyData(idx, debugs.top());
        debugs.pop();
        idx++;
    }
}
void OMInterpreter::logAnyData(int idx, std::any data)
{
    auto target = std::type_index(data.type());
    if (target == std::type_index(typeid(void *)))
    {
        logger.debug("{3}[{0}] {2}{1:#018x}{4} at heap tree", idx, (uint64_t)(void *)std::any_cast<void *>(data),
                     OMLogAnsiYellowLight, OMLogAnsiBlueLight, OMLogAnsiReset);
    }
    else if (target == std::type_index(typeid(std::shared_ptr<OMFrameMetadata>)))
    {
        auto fmd = std::any_cast<std::shared_ptr<OMFrameMetadata>>(data);
        logger.debug("{6}[{0}]{7} frame metadata {8}{1}.{2}{7}{9}{3}{7} + {5}{4}{7} ({5}{10}{7} + {12}{11}{7})", idx,
                     fmd->clazz, fmd->method, fmd->sig, fmd->offset, OMLogAnsiYellowLight, OMLogAnsiBlueLight,
                     OMLogAnsiReset, fmd->isNative ? OMLogAnsiMagentaLight : OMLogAnsiCyanLight, OMLogAnsiBlackLight,
                     fmd->locals->size(), fmd->allowedStackDepth, OMLogAnsiYellow);
    }
    else if (target == std::type_index(typeid(float)))
    {
        logger.debug("{3}[{0}] {2}{1}f{4}", idx, std::any_cast<float>(data), OMLogAnsiGreenLight, OMLogAnsiBlueLight,
                     OMLogAnsiReset);
    }
    else if (target == std::type_index(typeid(int)))
    {
        logger.debug("{3}[{0}] {2}{1}{4}", idx, std::any_cast<int>(data), OMLogAnsiGreenLight, OMLogAnsiBlueLight,
                     OMLogAnsiReset);
    }
    else if (target == std::type_index(typeid(OMArray<const char *>)))
    {
        auto data0 = std::any_cast<OMArray<const char *>>(data);
        std::string target;
        target += "[";
        for (uint32_t i = 0; i < data0.length; i++)
        {
            target.append(data0.data[i]);
            target.append(", ");
        }
        target.append("]");
        logger.debug("{2}[{0}]{3} array {4}{1}{3}", idx, target, OMLogAnsiBlueLight, OMLogAnsiReset,
                     OMLogAnsiGreenLight);
    }
    else if (target == std::type_index(typeid(OMLocalVariablePlaceholder)))
    {
        logger.debug("{1}[{0}] {2}uninitialized local variable{3}", idx, OMLogAnsiBlueLight, OMLogAnsiBlackLight,
                     OMLogAnsiReset);
    }
    else
    {
        logger.info("{1}[{0}] {2}???{3}", idx, OMLogAnsiBlueLight, OMLogAnsiBlackLight, OMLogAnsiReset);
    }
}
void OMInterpreter::operand_istore(uint64_t &offset, int data, std::shared_ptr<OMFrameMetadata> frame)
{
    *frame->locals->at(data) = stack.top();
    stack.pop();
    offset++;
}
void OMInterpreter::operand_invokespecial(uint64_t &offset, std::string clazz, std::string func, std::string desc)
{
    execute(clazz, func, desc, false);
    offset += 3;
}
void OMInterpreter::operand_invokevirtual(uint64_t &offset, std::string clazz, std::string func, std::string desc)
{
    execute(clazz, func, desc, false);
    offset += 3;
}
void OMInterpreter::operand_invokestatic(uint64_t &offset, std::string clazz, std::string func, std::string desc)
{
    execute(clazz, func, desc, true);
    offset += 3;
}
void OMInterpreter::operand_nop(uint64_t &offset)
{
    offset++;
}
void OMInterpreter::operand_iconst(uint64_t &offset, int data)
{
    stack.push(data);
    offset++;
}
void OMInterpreter::operand_new(uint64_t &offset, std::string type)
{
    if (!loader.classLoaded(type))
    {
        throw std::logic_error("class not loaded!");
    }
    auto id = memoryTree.allocateId();
    stack.push((void *)id);
    memoryTree.externalData[id] = type;
    memoryTree.allocate(id, 0);
    memoryTree.attach(heap::heapRoot, id);
    offset += 3;
}
void OMInterpreter::operand_dup(uint64_t &offset)
{
    stack.push(stack.top());
    offset++;
}
void OMInterpreter::operand_return()
{
    logger.debug("");
    logger.debug("STACK DETAILS:");
    uint64_t idx = 0;
    debugStack();
    while (true)
    {
        if (std::type_index(stack.top().type()) == std::type_index(typeid(void *)))
        {
            memoryTree.detach(heap::heapRoot, (uint64_t)(void *)std::any_cast<void *>(stack.top()));
        }

        if (std::type_index(stack.top().type()) == std::type_index(typeid(std::shared_ptr<OMFrameMetadata>)))
        {
            for (auto m : std::any_cast<std::shared_ptr<OMFrameMetadata>>(stack.top())->allocatedObjects)
            {
                memoryTree.detach(heap::heapRoot, (uint64_t)m);
                memoryTree.externalData.erase((uint64_t)m);
            }
            memoryTree.deconstructUnreachable();
            stack.pop();
            break;
        }

        stack.pop();
        idx++;
    }
}
void OMInterpreter::operand_pop(uint64_t &offset)
{
    stack.pop();
    offset++;
}
} // namespace openminecraft::vm::pixeltower
