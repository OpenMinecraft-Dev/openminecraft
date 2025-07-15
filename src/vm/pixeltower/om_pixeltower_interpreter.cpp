#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/log/om_log_ansi.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_frame_structdef.hpp"
#include <memory>
#include <vector>

using namespace openminecraft::util;
using namespace openminecraft::log::ansi;
using namespace openminecraft::vm::classfile;

namespace openminecraft::vm::pixeltower::runtime
{
OMInterpreter::OMInterpreter(OMPixelTower &tower) : tower(tower), logger("pixeltower/OMInterpreter", this)
{
}
OMInterpreter::~OMInterpreter()
{
}
OMResult<std::any, err::OMValidationError> OMInterpreter::execute(std::shared_ptr<OMClass> clazz, std::string method,
                                                                  std::string sig)
{
    for (auto &m : clazz->methods)
    {
        if (m->name == method && m->desc == sig)
        {
            return execute(clazz, m);
        }
    }
    if (clazz->superClass != nullptr)
    {
        return execute(clazz->superClass, method, sig);
    }
    return OMResult<std::any, err::OMValidationError>::err(
        {err::ClassLoader, "method not found", fmt::format("{}.{}{}", clazz->name, method, sig)});
}
OMResult<std::any, err::OMValidationError> OMInterpreter::execute(std::shared_ptr<OMClass> clazz,
                                                                  std::shared_ptr<OMMethodInfo> mi)
{
    auto frame = std::make_shared<OMFrameMetadata>(
        OMFrameMetadata{clazz, mi, nullptr, 0, std::vector<std::any>(mi->code->maxLocals)});

    {
        int temp = 0;
        auto result = bytecode::descriptor::decodeSignature(mi->desc, &temp);
        if (result.type == Err)
        {
            return OMResult<std::any, err::OMValidationError>::err(
                {err::Instructions,
                 fmt::format("unrecognized method descriptor at {}.{}{}", clazz->name, mi->name, mi->desc),
                 result.unwrap_err()});
        }
        auto args = result.unwrap().first.size();

        if ((mi->accessFlag & JVM_Acc_Static) == 0)
        {
            args++;
        }

        for (int argid = args - 1; argid >= 0; argid--)
        {
            frame->local[argid] = stack.top();
            stack.pop();
        }
    }

    stack.push(frame);

    {
        if (mi->code == nullptr)
        {
            return OMResult<std::any, err::OMValidationError>::err(
                {err::Instructions, "no bytecode here!", fmt::format("{}.{}{}", clazz->name, mi->name, mi->desc)});
        }

        auto codeArea = mi->code->code->data();
        frame->codePointer = codeArea;
        frame->codeLength = mi->code->codeLength;
        while (frame->offset < mi->code->codeLength)
        {
            switch (codeArea[frame->offset])
            {
            case op_invokestatic: {
                operand_invokestatic(frame);
                break;
            }
            case op_return: {
                operand_return(frame);
                return OMResult<std::any, err::OMValidationError>::ok(nullptr);
            }
            case op_new: {
                operand_new(frame);
                break;
            }
            default: {
                tower.debugStackStatus();
                return OMResult<std::any, err::OMValidationError>::err(
                    {err::Instructions, "instructions not implemented",
                     fmt::format("{}.{}{} + {}", clazz->name, mi->name, mi->desc, frame->offset)});
            }
            }
        }
    }

    return OMResult<std::any, err::OMValidationError>::err(
        {err::Instructions, "code heap overflow!",
         fmt::format("{}.{}{} + {}", clazz->name, mi->name, mi->desc, frame->offset)});
}
void OMInterpreter::operand_invokestatic(std::shared_ptr<OMFrameMetadata> frame)
{
    auto mrIdx = binary::be16ToNative(*(uint16_t *)(frame->codePointer + frame->offset + 1));
    auto temp1 = frame->clazz->mapping->at(mrIdx)->to<OMClassConstantMethodRef>();
    auto temp2 = frame->clazz->mapping->at(temp1->classIndex)->to<OMClassConstantClass>();
    auto temp3 = frame->clazz->mapping->at(temp1->nameAndTypeIndex)->to<OMClassConstantNameAndType>();

    auto cls = frame->clazz->mapping->at(temp2->nameIndex)->to<OMClassConstantUtf8>()->data;
    auto name = frame->clazz->mapping->at(temp3->nameIndex)->to<OMClassConstantUtf8>()->data;
    auto desc = frame->clazz->mapping->at(temp3->descIndex)->to<OMClassConstantUtf8>()->data;

    auto r = execute(cls, name, desc);
    if (r.type == util::Err)
    {
        throw r.unwrap_err();
    }

    frame->offset += 3;
}
void OMInterpreter::operand_return(std::shared_ptr<OMFrameMetadata> frame)
{
    while (std::any_cast<std::shared_ptr<OMFrameMetadata>>(stack.top()) != frame) {
        stack.pop();
    }

    stack.pop();
}
void OMInterpreter::operand_new(std::shared_ptr<OMFrameMetadata> frame)
{
    auto mrIdx = binary::be16ToNative(*(uint16_t *)(frame->codePointer + frame->offset + 1));
    auto temp1 = frame->clazz->mapping->at(mrIdx)->to<OMClassConstantClass>();

    auto cls = frame->clazz->mapping->at(temp1->nameIndex)->to<OMClassConstantUtf8>()->data;

    auto f = tower.fetchClass(cls);
    if (f.type == util::Err)
    {
        throw f.unwrap_err();
    }

    stack.push(tower.allocate(f.unwrap()));

    frame->offset += 3;
}
} // namespace openminecraft::vm::pixeltower::runtime
