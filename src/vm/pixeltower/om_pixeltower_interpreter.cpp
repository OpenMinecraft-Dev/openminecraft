#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_array_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_frame_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_memorymanager.hpp"
#include <any>
#include <cstring>
#include <memory>
#include <stack>
#include <string>
#include <thread>
#include <typeindex>
#include <vector>

using namespace openminecraft::util;
using namespace openminecraft::vm::classfile;
using namespace openminecraft::binary::hash;

namespace openminecraft::vm::pixeltower
{
extern std::string ARRAY_TYPE;
}

namespace openminecraft::vm::pixeltower::runtime
{
#define STACK_ACCESS stack[std::this_thread::get_id()]
#define SAFE_STACK_POP                                                                                                 \
    if (STACK_ACCESS.empty())                                                                                          \
    {                                                                                                                  \
        throw err::OMValidationError{err::Instructions, "Why the stack is empty?!", fetchCurrentPosition(frame)};      \
    }                                                                                                                  \
    STACK_ACCESS.pop();
#define STACK_CHECK                                                                                                    \
    if (STACK_ACCESS.empty())                                                                                          \
    {                                                                                                                  \
        throw err::OMValidationError{err::Instructions, "Why the stack is empty?!", fetchCurrentPosition(frame)};      \
    }

OMInterpreter::OMInterpreter(OMPixelTower &tower) : tower(tower), logger("pixeltower/OMInterpreter", this)
{
}
OMInterpreter::~OMInterpreter()
{
}
void OMInterpreter::execute(std::shared_ptr<OMClass> clazz, std::string method, std::string sig)
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
    throw err::OMValidationError{err::ClassLoader, "method not found",
                                 fmt::format("{}.{}{}", clazz->name, method, sig)};
}
void OMInterpreter::executeDynamic(std::string clazz, std::string method, std::string sig,
                                   std::shared_ptr<OMFrameMetadata> frame)
{
    std::stack<std::any> tempst;
    int temp = 0;
    auto result = bytecode::descriptor::decodeSignature(sig, &temp);
    if (result.type == Err)
    {
        throw err::OMValidationError{err::Instructions,
                                     fmt::format("unrecognized method descriptor at {}.{}{}", clazz, method, sig),
                                     result.unwrap_err()};
    }

    for (int i = 0; i < result.unwrap().first.size(); i++)
    {
        STACK_CHECK;
        tempst.push(STACK_ACCESS.top());
        SAFE_STACK_POP;
    }

    if (std::type_index(STACK_ACCESS.top().type()) == std::type_index(typeid(void *)))
    {
        auto obj = std::any_cast<void *>(STACK_ACCESS.top());
        if (obj == nullptr)
        {
            throw err::OMValidationError{err::Instructions,
                                         fmt::format("interface linking with nullptr at {}.{}{}", clazz, method, sig),
                                         result.unwrap_err()};
        }

        while (!tempst.empty())
        {
            STACK_ACCESS.push(tempst.top());
            tempst.pop();
        }

        return execute((*((OMClass **)obj))->name, method, sig);
    }
    else
    {
        throw err::OMValidationError{err::Instructions,
                                     fmt::format("unrecognized stack item at {}.{}{}", clazz, method, sig),
                                     result.unwrap_err()};
    }
}
void OMInterpreter::execute(std::shared_ptr<OMClass> clazz, std::shared_ptr<OMMethodInfo> mi)
{
    auto frame = std::make_shared<OMFrameMetadata>(
        OMFrameMetadata{clazz, mi, nullptr, 0, std::vector<std::any>(mi->code->maxLocals)});

    {
        int temp = 0;
        auto result = bytecode::descriptor::decodeSignature(mi->desc, &temp);
        if (result.type == Err)
        {
            throw err::OMValidationError{
                err::Instructions,
                fmt::format("unrecognized method descriptor at {}.{}{}", clazz->name, mi->name, mi->desc),
                result.unwrap_err()};
        }
        auto args = result.unwrap().first.size();
        std::vector<std::string> types;
        for (auto i : result.unwrap().first)
        {
            types.push_back(i);
        }

        if ((mi->accessFlag & JVM_Acc_Static) == 0)
        {
            args++;
            types.insert(types.begin(), "L" + clazz->name);
        }

        for (int argid = args - 1; argid >= 0; argid--)
        {
            STACK_CHECK;
            frame->local[argid] = STACK_ACCESS.top();
            checkType(frame, STACK_ACCESS.top(), types[argid]);
            SAFE_STACK_POP;
        }
    }

    STACK_ACCESS.push(frame);

    {
        if (mi->code == nullptr)
        {
            throw err::OMValidationError{err::Instructions, "no bytecode here!",
                                         fmt::format("{}.{}{}", clazz->name, mi->name, mi->desc)};
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

            case op_istore_n(0):
            case op_istore_n(1):
            case op_istore_n(2):
            case op_istore_n(3): {
                operand_istore_n(frame);
                break;
            }

            case op_astore_n(0):
            case op_astore_n(1):
            case op_astore_n(2):
            case op_astore_n(3): {
                operand_astore_n(frame);
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
                return;
            }

            case op_return: {
                operand_return(frame);
                return;
            }

            case op_putstatic: {
                operand_putstatic(frame);
                break;
            }

            case op_putfield: {
                operand_putfield(frame);
                break;
            }

            case op_invokestatic:
            case op_invokespecial:
            case op_invokevirtual: {
                operand_invokeany(frame);
                break;
            }

            case op_invokeinterface: {
                operand_invokeinterface(frame);
                break;
            }

            case op_new: {
                operand_new(frame);
                break;
            }

            default: {
                tower.debugStackStatus();
                throw err::OMValidationError{err::Instructions, "instructions not implemented",
                                             fetchCurrentPosition(frame)};
            }
            }
        }
    }

    throw err::OMValidationError{err::Instructions, "code heap overflow!", fetchCurrentPosition(frame)};
}
void OMInterpreter::checkType(std::shared_ptr<OMFrameMetadata> frame, std::any data, std::string desc)
{
    switch (binary::hash::hash_compile_time(desc.c_str()))
    {
        // In stack, these types are the same
    case "short"_hash:
    case "boolean"_hash:
    case "int"_hash:
    case "char"_hash:
    case "byte"_hash: {
        if (std::type_index(data.type()) != std::type_index(typeid(int)))
        {
            throw err::OMValidationError{
                err::Instructions,
                fmt::format("item type mismatch, required int (actually it contains data with native descriptor {})",
                            data.type().name()),
                fetchCurrentPosition(frame)};
        }
        break;
    }
    case "long"_hash: {
        if (std::type_index(data.type()) != std::type_index(typeid(int64_t)))
        {
            throw err::OMValidationError{
                err::Instructions,
                fmt::format("item type mismatch, required long (actually it contains data with native descriptor {})",
                            data.type().name()),
                fetchCurrentPosition(frame)};
        }
        break;
    }
    case "float"_hash: {
        if (std::type_index(data.type()) != std::type_index(typeid(float)))
        {
            throw err::OMValidationError{
                err::Instructions,
                fmt::format("item type mismatch, required float (actually it contains data with native descriptor {})",
                            data.type().name()),
                fetchCurrentPosition(frame)};
        }
        break;
    }
    case "double"_hash: {
        if (std::type_index(data.type()) != std::type_index(typeid(double)))
        {
            throw err::OMValidationError{
                err::Instructions,
                fmt::format("item type mismatch, required double (actually it contains data with native descriptor {})",
                            data.type().name()),
                fetchCurrentPosition(frame)};
        }
        break;
    }
    default: {
        if (std::type_index(data.type()) != std::type_index(typeid(void *)))
        {
            throw err::OMValidationError{
                err::Instructions,
                fmt::format(
                    "item type mismatch, required pointer (actually it contains data with native descriptor {})",
                    data.type().name()),
                fetchCurrentPosition(frame)};
        }

        void *ptr = std::any_cast<void *>(data);
        if (desc[0] == '[')
        {
            auto header = (OMArrayHeader *)ptr;
            if (header->classifierPointer != &ARRAY_TYPE)
            {
                throw err::OMValidationError{err::Instructions, fmt::format("not a array!"),
                                             fetchCurrentPosition(frame)};
            }
            if (desc[1] != header->dim)
            {
                throw err::OMValidationError{
                    err::Instructions,
                    fmt::format("array dimension mismatched, required {}, actually {}", (int)desc[1], header->dim),
                    fetchCurrentPosition(frame)};
            }

#define TYPE_MISMATCH                                                                                                  \
    throw err::OMValidationError{                                                                                      \
        err::Instructions,                                                                                             \
        fmt::format("array dimension mismatched, required {}, actually {}", (int)desc[1], fetchName(header->type)),    \
        fetchCurrentPosition(frame)};

            auto descp = std::string(desc.c_str()).substr(2);

            switch (hash_compile_time(descp.c_str()))
            {
            case "byte"_hash:
                if (header->type != Byte)
                {
                    TYPE_MISMATCH;
                }
                break;
            case "char"_hash:
                if (header->type != Char)
                {
                    TYPE_MISMATCH;
                }
                break;
            case "boolean"_hash:
                if (header->type != Boolean)
                {
                    TYPE_MISMATCH;
                }
                break;
            case "int"_hash:
                if (header->type != Int)
                {
                    TYPE_MISMATCH;
                }
                break;
            case "long"_hash:
                if (header->type != Long)
                {
                    TYPE_MISMATCH;
                }
            case "short"_hash:
                if (header->type != Short)
                {
                    TYPE_MISMATCH;
                }
                break;
            case "double"_hash:
                if (header->type != Double)
                {
                    TYPE_MISMATCH;
                }
                break;
            case "float"_hash:
                if (header->type != Float)
                {
                    TYPE_MISMATCH;
                }
                break;
            default: {
                if (descp[0] == 'L')
                {
                    auto type = tower.fetchClass(std::string(desc.c_str()).substr(3));
                    if (!tower.classloader->isClassCompat(header->classPointer, type))
                    {
                        throw err::OMValidationError{
                            err::Instructions,
                            fmt::format("class type incomptiable ({} and {})", header->classPointer->name, type->name),
                            fetchCurrentPosition(frame)};
                    }
                    return;
                }
                throw err::OMValidationError{err::Instructions, fmt::format("unknown descriptor {}", desc),
                                             fetchCurrentPosition(frame)};
            }
            }
        }
        else if (desc[0] == 'L')
        {
            auto src = *((OMClass **)ptr);
            if (((OMArrayHeader *)ptr)->classifierPointer == &ARRAY_TYPE || src == nullptr)
            {
                throw err::OMValidationError{err::Instructions, fmt::format("not an object!"),
                                             fetchCurrentPosition(frame)};
            }
            auto type = tower.fetchClass(std::string(desc.c_str()).substr(1));
            if (!tower.classloader->isClassCompat(src, type))
            {
                throw err::OMValidationError{err::Instructions,
                                             fmt::format("class type incomptiable ({} and {})", src->name, type->name),
                                             fetchCurrentPosition(frame)};
            }
        }
        else
        {
            throw err::OMValidationError{err::Instructions, fmt::format("unknown descriptor {}", desc),
                                         fetchCurrentPosition(frame)};
        }
    }
    }
}
std::string OMInterpreter::fetchName(OMArrayType type)
{
    switch (type)
    {
    case Byte:
        return "byte";
    case Char:
        return "char";
    case Short:
        return "short";
    case Int:
        return "int";
    case Long:
        return "long";
    case Boolean:
        return "boolean";
    case Double:
        return "double";
    case Float:
        return "float";
    case Reference:
        return "ref";
    }
}
// TODO: other types
void OMInterpreter::writeStackTop(void *target, std::string desc, std::shared_ptr<OMFrameMetadata> frame)
{
    STACK_CHECK;
    auto it = std::type_index(STACK_ACCESS.top().type());

    if (it == std::type_index(typeid(int)))
    {
        *((int *)target) = std::any_cast<int>(STACK_ACCESS.top());
    }
    else if (it == std::type_index(typeid(void *)))
    {
        *((void **)target) = std::any_cast<void *>(STACK_ACCESS.top());
    }
    else
    {
        throw err::OMValidationError{err::Instructions, fmt::format("unknown stack top data"),
                                     fetchCurrentPosition(frame)};
    }

    SAFE_STACK_POP;
}
std::string OMInterpreter::fetchCurrentPosition(std::shared_ptr<OMFrameMetadata> frame)
{
    return fmt::format("{}.{}{} + {}", frame->clazz->name, frame->method->name, frame->method->desc, frame->offset);
}
void OMInterpreter::operand_ireturn(std::shared_ptr<OMFrameMetadata> frame)
{
    STACK_CHECK;
    auto ret = STACK_ACCESS.top();
    popFrame(frame);
    STACK_ACCESS.push(ret);
}
void OMInterpreter::operand_putfield(std::shared_ptr<OMFrameMetadata> frame)
{
    STACK_CHECK;
    auto mrIdx = binary::be16ToNative(*(uint16_t *)(frame->codePointer + frame->offset + 1));
    auto temp1 = frame->clazz->mapping->at(mrIdx)->to<OMClassConstantFieldRef>();
    auto temp2 = frame->clazz->mapping->at(temp1->classIndex)->to<OMClassConstantClass>();
    auto temp3 = frame->clazz->mapping->at(temp1->nameAndTypeIndex)->to<OMClassConstantNameAndType>();

    auto cls = frame->clazz->mapping->at(temp2->nameIndex)->to<OMClassConstantUtf8>()->data;
    auto name = frame->clazz->mapping->at(temp3->nameIndex)->to<OMClassConstantUtf8>()->data;
    auto desc = frame->clazz->mapping->at(temp3->descIndex)->to<OMClassConstantUtf8>()->data;

    auto res = tower.fetchClass(cls);

    frame->offset += 3;
    auto value = STACK_ACCESS.top();
    SAFE_STACK_POP;
    auto item = STACK_ACCESS.top();
    SAFE_STACK_POP;
    STACK_ACCESS.push(value);
    if (std::type_index(item.type()) != std::type_index(typeid(void *)))
    {
        throw err::OMValidationError{err::Instructions, "unknown stack item type", fetchCurrentPosition(frame)};
    }
    for (auto fi : res->fields)
    {
        if (fi->name == name && (fi->accessFlag & JVM_Acc_Static) == 0)
        {
            writeStackTop(OBJECT_ACCESS(std::any_cast<void *>(item), fi->offset), fi->desc, frame);
            return;
        }
    }

    throw err::OMValidationError{err::Instructions,
                                 fmt::format("static field not found for {} with name {}", cls, name),
                                 fetchCurrentPosition(frame)};
}
void OMInterpreter::operand_putstatic(std::shared_ptr<OMFrameMetadata> frame)
{
    auto mrIdx = binary::be16ToNative(*(uint16_t *)(frame->codePointer + frame->offset + 1));
    auto temp1 = frame->clazz->mapping->at(mrIdx)->to<OMClassConstantFieldRef>();
    auto temp2 = frame->clazz->mapping->at(temp1->classIndex)->to<OMClassConstantClass>();
    auto temp3 = frame->clazz->mapping->at(temp1->nameAndTypeIndex)->to<OMClassConstantNameAndType>();

    auto cls = frame->clazz->mapping->at(temp2->nameIndex)->to<OMClassConstantUtf8>()->data;
    auto name = frame->clazz->mapping->at(temp3->nameIndex)->to<OMClassConstantUtf8>()->data;
    auto desc = frame->clazz->mapping->at(temp3->descIndex)->to<OMClassConstantUtf8>()->data;

    auto res = tower.fetchClass(cls);

    frame->offset += 3;
    for (auto fi : res->fields)
    {
        if (fi->name == name && (fi->accessFlag & JVM_Acc_Static))
        {
            writeStackTop(OBJECT_ACCESS(res->staticFieldBlock, fi->offset), fi->desc, frame);
            return;
        }
    }

    throw err::OMValidationError{err::Instructions,
                                 fmt::format("static field not found for {} with name {}", cls, name),
                                 fetchCurrentPosition(frame)};
}
void OMInterpreter::operand_istore_n(std::shared_ptr<OMFrameMetadata> frame)
{
    STACK_CHECK;
    auto item = STACK_ACCESS.top();
    SAFE_STACK_POP;
    frame->local[frame->codePointer[frame->offset] - op_istore_n(0)] = item;
    frame->offset++;
}
void OMInterpreter::operand_invokeinterface(std::shared_ptr<OMFrameMetadata> frame)
{
    auto mrIdx = binary::be16ToNative(*(uint16_t *)(frame->codePointer + frame->offset + 1));
    auto temp1 = frame->clazz->mapping->at(mrIdx)->to<OMClassConstantMethodRef>();
    auto temp2 = frame->clazz->mapping->at(temp1->classIndex)->to<OMClassConstantClass>();
    auto temp3 = frame->clazz->mapping->at(temp1->nameAndTypeIndex)->to<OMClassConstantNameAndType>();

    auto cls = frame->clazz->mapping->at(temp2->nameIndex)->to<OMClassConstantUtf8>()->data;
    auto name = frame->clazz->mapping->at(temp3->nameIndex)->to<OMClassConstantUtf8>()->data;
    auto desc = frame->clazz->mapping->at(temp3->descIndex)->to<OMClassConstantUtf8>()->data;

    executeDynamic(cls, name, desc, frame);

    frame->offset += 5;
}
void OMInterpreter::operand_astore_n(std::shared_ptr<OMFrameMetadata> frame)
{
    STACK_CHECK;
    auto item = STACK_ACCESS.top();
    SAFE_STACK_POP;
    frame->local[frame->codePointer[frame->offset] - op_astore_n(0)] = item;
    frame->offset++;
}
void OMInterpreter::operand_iconst_n(std::shared_ptr<OMFrameMetadata> frame)
{
    STACK_ACCESS.push((int)(frame->codePointer[frame->offset] - op_iconst_i(0)));
    frame->offset++;
}
void OMInterpreter::operand_lconst_n(std::shared_ptr<OMFrameMetadata> frame)
{
    STACK_ACCESS.push((int64_t)(frame->codePointer[frame->offset] - op_lconst_l(0)));
    frame->offset++;
}
void OMInterpreter::operand_fconst_n(std::shared_ptr<OMFrameMetadata> frame)
{
    STACK_ACCESS.push((float)(frame->codePointer[frame->offset] - op_fconst_f(0)));
    frame->offset++;
}
void OMInterpreter::operand_dconst_n(std::shared_ptr<OMFrameMetadata> frame)
{
    STACK_ACCESS.push((double)(frame->codePointer[frame->offset] - op_dconst_d(0)));
    frame->offset++;
}
void OMInterpreter::operand_if_acmpne(std::shared_ptr<OMFrameMetadata> frame)
{
    STACK_CHECK;
    auto a1 = STACK_ACCESS.top();
    SAFE_STACK_POP;
    auto a2 = STACK_ACCESS.top();
    SAFE_STACK_POP;

    if (std::type_index(a1.type()) != std::type_index(typeid(void *)))
    {
        throw err::OMValidationError{err::Instructions, "stack element type mismatch, required reference for slot 0",
                                     fetchCurrentPosition(frame)};
    }

    if (std::type_index(a2.type()) != std::type_index(typeid(void *)))
    {
        throw err::OMValidationError{err::Instructions, "stack element type mismatch, required reference for slot 1",
                                     fetchCurrentPosition(frame)};
    }

    if (std::any_cast<void *>(a1) != std::any_cast<void *>(a2))
    {
        frame->offset += binary::be16ToNative(*(uint16_t *)(frame->codePointer + frame->offset + 1));
    }
    else
    {
        frame->offset += 3;
    }
}
void OMInterpreter::operand_aload_n(std::shared_ptr<OMFrameMetadata> frame)
{
    STACK_ACCESS.push(frame->local[frame->codePointer[frame->offset] - op_aload_n(0)]);
    frame->offset++;
}
void OMInterpreter::operand_invokeany(std::shared_ptr<OMFrameMetadata> frame)
{
    auto mrIdx = binary::be16ToNative(*(uint16_t *)(frame->codePointer + frame->offset + 1));
    auto temp1 = frame->clazz->mapping->at(mrIdx)->to<OMClassConstantMethodRef>();
    auto temp2 = frame->clazz->mapping->at(temp1->classIndex)->to<OMClassConstantClass>();
    auto temp3 = frame->clazz->mapping->at(temp1->nameAndTypeIndex)->to<OMClassConstantNameAndType>();

    auto cls = frame->clazz->mapping->at(temp2->nameIndex)->to<OMClassConstantUtf8>()->data;
    auto name = frame->clazz->mapping->at(temp3->nameIndex)->to<OMClassConstantUtf8>()->data;
    auto desc = frame->clazz->mapping->at(temp3->descIndex)->to<OMClassConstantUtf8>()->data;

    auto clss = tower.fetchClass(cls);

    execute(clss, name, desc);

    frame->offset += 3;
}
void OMInterpreter::operand_dup(std::shared_ptr<OMFrameMetadata> frame)
{
    STACK_CHECK;
    STACK_ACCESS.push(STACK_ACCESS.top());
    frame->offset++;
}
void OMInterpreter::operand_return(std::shared_ptr<OMFrameMetadata> frame)
{
    return popFrame(frame);
}
void OMInterpreter::popFrame(std::shared_ptr<OMFrameMetadata> frame)
{
    STACK_CHECK;
    while (std::type_index(STACK_ACCESS.top().type()) != std::type_index(typeid(std::shared_ptr<OMFrameMetadata>)) ||
           std::any_cast<std::shared_ptr<OMFrameMetadata>>(STACK_ACCESS.top()) != frame)
    {
        SAFE_STACK_POP;
        if (STACK_ACCESS.empty())
        {
            throw err::OMValidationError{err::Instructions, "whole operator stack is popped! tower is crashing!",
                                         fetchCurrentPosition(frame)};
        }
    }

    SAFE_STACK_POP;
}
void OMInterpreter::operand_new(std::shared_ptr<OMFrameMetadata> frame)
{
    auto mrIdx = binary::be16ToNative(*(uint16_t *)(frame->codePointer + frame->offset + 1));
    auto temp1 = frame->clazz->mapping->at(mrIdx)->to<OMClassConstantClass>();

    auto cls = frame->clazz->mapping->at(temp1->nameIndex)->to<OMClassConstantUtf8>()->data;

    STACK_ACCESS.push(tower.allocate(tower.fetchClass(cls)));

    frame->offset += 3;
}
void OMInterpreter::operand_nop(std::shared_ptr<OMFrameMetadata> frame)
{
    frame->offset++;
}
void OMInterpreter::operand_aconst_null(std::shared_ptr<OMFrameMetadata> frame)
{
    STACK_ACCESS.push((void *)nullptr);
    frame->offset++;
}
void OMInterpreter::operand_pop(std::shared_ptr<OMFrameMetadata> frame)
{
    SAFE_STACK_POP;
    frame->offset++;
}
void *OMInterpreter::newString(std::string data)
{
    auto cls = tower.fetchClass("java/lang/String");

    void *rawarr = tower.allocateArray(Byte, data.size());
    char *arrdata = ((char *)rawarr) + sizeof(OMArrayHeader);
    memcpy(arrdata, data.c_str(), data.size());
    void *str = tower.allocate(cls);
    STACK_ACCESS.push(str);
    STACK_ACCESS.push(rawarr);
    execute(cls, "<init>", "([B)V");
    return str;
}
} // namespace openminecraft::vm::pixeltower::runtime
