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
#include <typeindex>
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
            case op_nop: {
                operand_nop(frame);
                break;
            }

            case op_aconst_null: {
                operand_aconst_null(frame);
                break;
            }
            case op_iconst_i(-1):
            case op_iconst_i(0):
            case op_iconst_i(1):
            case op_iconst_i(2):
            case op_iconst_i(3):
            case op_iconst_i(4):
            case op_iconst_i(5): {
                operand_iconst_n(frame);
                break;
            }

            case op_lconst_l(0):
            case op_lconst_l(1): {
                operand_lconst_n(frame);
                break;
            }

            case op_fconst_f(0):
            case op_fconst_f(1):
            case op_fconst_f(2): {
                operand_fconst_n(frame);
                break;
            }

            case op_dconst_d(0):
            case op_dconst_d(1): {
                operand_dconst_n(frame);
                break;
            }

            case op_aload_n(0):
            case op_aload_n(1):
            case op_aload_n(2):
            case op_aload_n(3): {
                operand_aload_n(frame);
                break;
            }

            case op_pop: {
                operand_pop(frame);
                break;
            }

            case op_dup: {
                operand_dup(frame);
                break;
            }

            case op_if_acmpne: {
                operand_if_acmpne(frame);
                break;
            }

            case op_ireturn: {
                operand_ireturn(frame);
                return OMResult<std::any, err::OMValidationError>::ok(nullptr);
            }

            case op_return: {
                operand_return(frame);
                return OMResult<std::any, err::OMValidationError>::ok(nullptr);
            }

            case op_invokestatic:
            case op_invokespecial:
            case op_invokevirtual: {
                auto res = operand_invokeany(frame);
                if (res.type == Err)
                {
                    return OMResult<std::any, err::OMValidationError>::err(res.unwrap_err());
                }
                break;
            }

            case op_new: {
                operand_new(frame);
                break;
            }

            default: {
                tower.debugStackStatus();
                return OMResult<std::any, err::OMValidationError>::err(
                    {err::Instructions, "instructions not implemented", fetchCurrentPosition(frame)});
            }
            }
        }
    }

    return OMResult<std::any, err::OMValidationError>::err(
        {err::Instructions, "code heap overflow!", fetchCurrentPosition(frame)});
}
std::string OMInterpreter::fetchCurrentPosition(std::shared_ptr<OMFrameMetadata> frame)
{
    return fmt::format("{}.{}{} + {}", frame->clazz->name, frame->method->name, frame->method->desc, frame->offset);
}
util::OMResult<std::any, err::OMValidationError> OMInterpreter::operand_ireturn(std::shared_ptr<OMFrameMetadata> frame)
{
    auto ret = stack.top();
    auto l = popFrame(frame);
    stack.push(ret);
    return l;
}
void OMInterpreter::operand_iconst_n(std::shared_ptr<OMFrameMetadata> frame)
{
    stack.push((int)(frame->codePointer[frame->offset] - op_iconst_i(0)));
    frame->offset++;
}
void OMInterpreter::operand_lconst_n(std::shared_ptr<OMFrameMetadata> frame)
{
    stack.push((int64_t)(frame->codePointer[frame->offset] - op_lconst_l(0)));
    frame->offset++;
}
void OMInterpreter::operand_fconst_n(std::shared_ptr<OMFrameMetadata> frame)
{
    stack.push((float)(frame->codePointer[frame->offset] - op_fconst_f(0)));
    frame->offset++;
}
void OMInterpreter::operand_dconst_n(std::shared_ptr<OMFrameMetadata> frame)
{
    stack.push((double)(frame->codePointer[frame->offset] - op_dconst_d(0)));
    frame->offset++;
}
util::OMResult<std::any, err::OMValidationError> OMInterpreter::operand_if_acmpne(
    std::shared_ptr<OMFrameMetadata> frame)
{
    auto a1 = stack.top();
    stack.pop();
    auto a2 = stack.top();
    stack.pop();

    if (std::type_index(a1.type()) != std::type_index(typeid(void *)))
    {
        return util::OMResult<std::any, err::OMValidationError>::err(
            {err::Instructions, "stack element type mismatch, required reference for slot 0",
             fetchCurrentPosition(frame)});
    }

    if (std::type_index(a2.type()) != std::type_index(typeid(void *)))
    {
        return util::OMResult<std::any, err::OMValidationError>::err(
            {err::Instructions, "stack element type mismatch, required reference for slot 1",
             fetchCurrentPosition(frame)});
    }

    if (std::any_cast<void *>(a1) != std::any_cast<void *>(a2))
    {
        frame->offset += binary::be16ToNative(*(uint16_t *)(frame->codePointer + frame->offset + 1));
    }
    else
    {
        frame->offset += 3;
    }

    return util::OMResult<std::any, err::OMValidationError>::ok(nullptr);
}
void OMInterpreter::operand_aload_n(std::shared_ptr<OMFrameMetadata> frame)
{
    stack.push(frame->local[frame->codePointer[frame->offset] - op_aload_n(0)]);
    frame->offset++;
}
OMResult<std::any, err::OMValidationError> OMInterpreter::operand_invokeany(std::shared_ptr<OMFrameMetadata> frame)
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
        return OMResult<std::any, err::OMValidationError>::err(r.unwrap_err());
    }

    frame->offset += 3;

    return OMResult<std::any, err::OMValidationError>::ok(nullptr);
}
void OMInterpreter::operand_dup(std::shared_ptr<OMFrameMetadata> frame)
{
    stack.push(stack.top());
    frame->offset++;
}
util::OMResult<std::any, err::OMValidationError> OMInterpreter::operand_return(std::shared_ptr<OMFrameMetadata> frame)
{
    return popFrame(frame);
}
util::OMResult<std::any, err::OMValidationError> OMInterpreter::popFrame(std::shared_ptr<OMFrameMetadata> frame)
{
    while (std::type_index(stack.top().type()) != std::type_index(typeid(std::shared_ptr<OMFrameMetadata>)) ||
           std::any_cast<std::shared_ptr<OMFrameMetadata>>(stack.top()) != frame)
    {
        stack.pop();
        if (stack.empty())
        {
            return util::OMResult<std::any, err::OMValidationError>::err(
                {err::Instructions, "whole operator stack is popped! tower is crashing!", fetchCurrentPosition(frame)});
        }
    }

    stack.pop();
    return util::OMResult<std::any, err::OMValidationError>::ok(nullptr);
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
void OMInterpreter::operand_nop(std::shared_ptr<OMFrameMetadata> frame)
{
    frame->offset++;
}
void OMInterpreter::operand_aconst_null(std::shared_ptr<OMFrameMetadata> frame)
{
    stack.push((void *)nullptr);
    frame->offset++;
}
void OMInterpreter::operand_pop(std::shared_ptr<OMFrameMetadata> frame)
{
    stack.pop();
    frame->offset++;
}
} // namespace openminecraft::vm::pixeltower::runtime
