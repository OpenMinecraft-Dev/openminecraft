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
#include <any>
#include <memory>
#include <stdexcept>
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
void OMInterpreter::executeBytecode(std::shared_ptr<OMClassFile> f, OMClassAttrCode *codeWrap)
{
    auto frame = std::any_cast<std::shared_ptr<OMFrameMetadata>>(stack.top());
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

    for (int i = 0; i < res.unwrap().first.size() + !isStatic; i++)
    {
        local.push_back(stack.top());
        stack.pop();
    }

    auto frame = std::make_shared<OMFrameMetadata>(OMFrameMetadata{cls, func, desc, 0});
    stack.push(frame);

    localOffset += codeWrap->maxLocals;
    executeBytecode(f, codeWrap);
    localOffset -= codeWrap->maxLocals;

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
        auto frame = std::make_shared<OMFrameMetadata>(OMFrameMetadata{clazz, func, desc, 0});
        stack.push(frame);
        loadedNativeClasses[clazz]->invoke(func, stack, args);
        if (std::type_index(stack.top().type()) == std::type_index(typeid(OMFrameMetadata)))
        {
            throw std::logic_error("native impl not working!");
        }
    }

    uint64_t idx = 0;
    auto printData = [&](std::any data) {
        auto target = std::type_index(data.type());
        logger.info("{}", target.name());
        if (target == std::type_index(typeid(void *)))
        {
            logger.info("{3}[{0}] {2}{1:#018x}{4} at heap tree", idx, (uint64_t)(void *)std::any_cast<void *>(data),
                        OMLogAnsiYellowLight, OMLogAnsiBlueLight, OMLogAnsiReset);
        }
        else if (target == std::type_index(typeid(std::shared_ptr<OMFrameMetadata>)))
        {
            auto fmd = std::any_cast<std::shared_ptr<OMFrameMetadata>>(data);
            logger.info("{6}[{0}]{7} frame metadata {8}{1}.{2}{7}{9}{3}{7} + {5}{4}{7}", idx, fmd->clazz, fmd->method,
                        fmd->sig, fmd->offset, OMLogAnsiYellowLight, OMLogAnsiBlueLight, OMLogAnsiReset,
                        OMLogAnsiCyanLight, OMLogAnsiBlackLight);
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
    };
    while (true)
    {
        printData(stack.top());

        if (std::type_index(stack.top().type()) == std::type_index(typeid(std::shared_ptr<OMFrameMetadata>)))
        {
            stack.pop();
            std::vector<std::any> cache;
            while (!stack.empty())
            {
                cache.push_back(stack.top());
                idx++;
                printData(stack.top());
                stack.pop();
            }
            for (int i = cache.size() - 1; i >= 0; i--)
            {
                stack.push(cache[i]);
            }
            break;
        }

        stack.pop();
        idx++;
    }
    logger.info("......................");
    return;
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
