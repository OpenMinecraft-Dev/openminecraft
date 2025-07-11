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
OMInterpreter::OMInterpreter() : logger("pixeltower/OMInterpreter", this)
{
}
OMInterpreter::~OMInterpreter()
{
}
void OMInterpreter::loadClass(std::string name)
{
    logger.info("loading class {}", name);
    if (name == "java/lang/Object")
    {
        loadedNativeClasses["java/lang/Object"] = std::make_shared<stdlib::java::lang::Object>();
    }
}
void OMInterpreter::loadClass(std::shared_ptr<vm::classfile::OMClassFile> f)
{
    auto name =
        f->mapping[f->mapping[f->thisClass]->to<OMClassConstantClass>()->nameIndex]->to<OMClassConstantUtf8>()->data;
    logger.info("loading class {}", name);
    loadedClasses[name] = f;
}
void OMInterpreter::executeBytecode(std::shared_ptr<OMClassFile> f, OMClassAttrCode *codeWrap,
                                    std::shared_ptr<OMFrameMetadata> frame)
{
    while (frame->offset < codeWrap->codeLength)
    {
        switch (codeWrap->code[frame->offset])
        {
        case op_nop: {
            operand_nop(frame->offset);
            break;
        }
        case op_new: {
            auto id = binary::be16ToNative(*(uint16_t *)(codeWrap->code.data() + frame->offset + 1));
            auto type =
                f->mapping[f->mapping[id]->to<OMClassConstantClass>()->nameIndex]->to<OMClassConstantUtf8>()->data;
            operand_new(frame->offset, type);
            break;
        }
        case op_dup: {
            operand_dup(frame->offset);
            break;
        }
        case op_invokespecial: {
            auto id = binary::be16ToNative(*(uint16_t *)(codeWrap->code.data() + frame->offset + 1));
            auto temp1 = f->mapping[id]->to<OMClassConstantMethodRef>();
            auto cls = f->mapping[f->mapping[temp1->classIndex]->to<OMClassConstantClass>()->nameIndex]
                           ->to<OMClassConstantUtf8>()
                           ->data;
            auto temp2 = f->mapping[temp1->nameAndTypeIndex]->to<OMClassConstantNameAndType>();
            auto name = f->mapping[temp2->nameIndex]->to<OMClassConstantUtf8>()->data;
            auto desc = f->mapping[temp2->descIndex]->to<OMClassConstantUtf8>()->data;
            operand_invokespecial(frame->offset, cls, name, desc);
            break;
        }
        case op_invokevirtual: {
            auto id = binary::be16ToNative(*(uint16_t *)(codeWrap->code.data() + frame->offset + 1));
            auto temp1 = f->mapping[id]->to<OMClassConstantMethodRef>();
            auto cls = f->mapping[f->mapping[temp1->classIndex]->to<OMClassConstantClass>()->nameIndex]
                           ->to<OMClassConstantUtf8>()
                           ->data;
            auto temp2 = f->mapping[temp1->nameAndTypeIndex]->to<OMClassConstantNameAndType>();
            auto name = f->mapping[temp2->nameIndex]->to<OMClassConstantUtf8>()->data;
            auto desc = f->mapping[temp2->descIndex]->to<OMClassConstantUtf8>()->data;
            operand_invokevirtual(frame->offset, cls, name, desc);
            break;
        }
        case op_invokestatic: {
            auto id = binary::be16ToNative(*(uint16_t *)(codeWrap->code.data() + frame->offset + 1));
            auto temp1 = f->mapping[id]->to<OMClassConstantMethodRef>();
            auto cls = f->mapping[f->mapping[temp1->classIndex]->to<OMClassConstantClass>()->nameIndex]
                           ->to<OMClassConstantUtf8>()
                           ->data;
            auto temp2 = f->mapping[temp1->nameAndTypeIndex]->to<OMClassConstantNameAndType>();
            auto name = f->mapping[temp2->nameIndex]->to<OMClassConstantUtf8>()->data;
            auto desc = f->mapping[temp2->descIndex]->to<OMClassConstantUtf8>()->data;
            operand_invokestatic(frame->offset, cls, name, desc);
            break;
        }
        case op_return: {
            return;
        }
        default:
            logger.warn("unknown instruction!");
            return;
        }
    }
}
bool OMInterpreter::findAndExecuteBytecode(std::string clazz, std::string func, std::string desc, bool isStatic)
{
    auto f = loadedClasses[clazz];
    if (f == nullptr)
    {
        return false;
    }

    OMClassAttrCode *codeWrap;
    for (auto method : f->methods)
    {
        auto name = f->mapping[method->nameIndex]->to<OMClassConstantUtf8>()->data;
        if (func == name && f->mapping[method->descIndex]->to<OMClassConstantUtf8>()->data == desc)
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
    return false;
execute:
    int temp = 0;
    auto res = bytecode::descriptor::decodeSignature(desc, &temp);
    switch (res.type)
    {
    case Ok:
        goto invoke_main;
    case Err:
        throw std::logic_error(res.unwrap_err());
    }

invoke_main:
    auto cls =
        f->mapping[f->mapping[f->thisClass]->to<OMClassConstantClass>()->nameIndex]->to<OMClassConstantUtf8>()->data;

    std::vector<std::any> args;
    for (int i = 0; i < res.unwrap().first.size() + !isStatic; i++)
    {
        args.push_back(stack.top());
        stack.pop();
    }

    auto frame = std::make_shared<OMFrameMetadata>(
        OMFrameMetadata{cls, func, desc, 0, codeWrap->maxStack, std::make_shared<std::vector<std::any *>>()});
    stack.push(frame);

    while (args.size() < codeWrap->maxLocals)
    {
        args.push_back(OMLocalVariablePlaceholder());
    }
    for (int i = args.size() - 1; i >= 0; i--)
    {
        stack.push(args[i]);
        frame->locals->push_back(&stack.top());
    }
    std::reverse(frame->locals->begin(), frame->locals->end());

    executeBytecode(f, codeWrap, frame);

    return true;
}
void OMInterpreter::execute(std::string clazz, std::string func, std::string desc, bool isStatic)
{
    if (!findAndExecuteBytecode(clazz, func, desc, isStatic))
    {
        int temp = 0;
        auto res = bytecode::descriptor::decodeSignature(desc, &temp);
        switch (res.type)
        {
        case Ok:
            goto invoke_main;
        case Err:
            throw std::logic_error(res.unwrap_err());
        }

    invoke_main:
        auto argLength = res.unwrap().first.size();
        std::vector<std::any> args;
        for (int i = 0; i < argLength + !isStatic; i++)
        {
            args.push_back(stack.top());
            stack.pop();
        }
        if (loadedNativeClasses[clazz] == nullptr)
        {
            throw std::logic_error("native class not found!");
        }

        for (int i = args.size() - 1; i >= 0; i--)
        {
            stack.push(args[i]);
        }

        loadedNativeClasses[clazz]->invoke(func, stack);
        if (std::type_index(stack.top().type()) == std::type_index(typeid(OMFrameMetadata)))
        {
            throw std::logic_error("native impl not working!");
        }
        return;
    }

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
            stack.pop();
            break;
        }

        stack.pop();
        idx++;
    }
    logger.info("<stack root>");
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
        logger.info("{3}[{0}] {2}{1:#018x}{4} at heap tree", idx, (uint64_t)(void *)std::any_cast<void *>(data),
                    OMLogAnsiYellowLight, OMLogAnsiBlueLight, OMLogAnsiReset);
    }
    else if (target == std::type_index(typeid(std::shared_ptr<OMFrameMetadata>)))
    {
        auto fmd = std::any_cast<std::shared_ptr<OMFrameMetadata>>(data);
        logger.info("{6}[{0}]{7} frame metadata {8}{1}.{2}{7}{9}{3}{7} + {5}{4}{7} ({5}{10}{7} + {12}{11}{7})", idx,
                    fmd->clazz, fmd->method, fmd->sig, fmd->offset, OMLogAnsiYellowLight, OMLogAnsiBlueLight,
                    OMLogAnsiReset, OMLogAnsiCyanLight, OMLogAnsiBlackLight, fmd->locals->size(),
                    fmd->allowedStackDepth, OMLogAnsiYellow);
    }
    else if (target == std::type_index(typeid(float)))
    {
        logger.info("{3}[{0}] {2}{1}f{4}", idx, std::any_cast<float>(data), OMLogAnsiGreenLight, OMLogAnsiBlueLight,
                    OMLogAnsiReset);
    }
    else if (target == std::type_index(typeid(int)))
    {
        logger.info("{3}[{0}] {2}{1}{4}", idx, std::any_cast<int>(data), OMLogAnsiGreenLight, OMLogAnsiBlueLight,
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
        logger.info("{2}[{0}]{3} array {4}{1}{3}", idx, target, OMLogAnsiBlueLight, OMLogAnsiReset,
                    OMLogAnsiGreenLight);
    }
    else if (target == std::type_index(typeid(OMLocalVariablePlaceholder)))
    {
        logger.info("{1}[{0}] {2}uninitialized local variable{3}", idx, OMLogAnsiBlueLight, OMLogAnsiBlackLight,
                    OMLogAnsiReset);
    }
    else
    {
        logger.info("{1}[{0}] {2}???{3}", idx, OMLogAnsiBlueLight, OMLogAnsiBlackLight, OMLogAnsiReset);
    }
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
void OMInterpreter::operand_new(uint64_t &offset, std::string type)
{
    loadClass(type);
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
