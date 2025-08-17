#include "openminecraft/vm/pixeltower/v3/om_pixeltower_validator.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include <cstdint>
#include <memory>
#include <stack>
#include <unordered_map>
#include <vector>

using namespace openminecraft::vm::classfile;
using namespace openminecraft::binary::hash;

constexpr int intItem = 0x0001;
constexpr int floatItem = 0x0002;
constexpr int longItem = 0x0004;
constexpr int doubleItem = 0x0008;
constexpr int refItem = 0x0010;
constexpr int slot2Item = 0x0020;

namespace openminecraft::vm::pixeltower::v3
{
OMValidator::OMValidator() : logger("OMValidator", this)
{
}

void OMValidator::validate(std::shared_ptr<OMClassFile> file, std::string name)
{
    validateConstantPool(file, name);

    for (auto m : file->methods)
    {
        std::map<int, bool> d;
        checkMethod(file, m, name, d);
    }
}
void OMValidator::checkRecursively(std::shared_ptr<OMClassFile> file, uint16_t id, std::string name,
                                   OMClassConstantType type)
{
    if (!file->mapping.count(id))
    {
        throw err::OMValidationError(err::Instructions, fmt::format("undefined constant with id {}", id), name);
    }
    auto c = file->mapping[id];
    if (c->type() != type)
    {
        throw err::OMValidationError{err::ConstantPool, fmt::format("constant type mismatch at {}", (int)id), name};
    }

    switch (c->type())
    {
    case classfile::OMClassConstantType::Utf8:
    case classfile::OMClassConstantType::Integer:
    case classfile::OMClassConstantType::Float:
    case classfile::OMClassConstantType::Long:
    case classfile::OMClassConstantType::Double:
        // geopelia: primitives!
        break;
    case classfile::OMClassConstantType::Class: {
        auto nid = c->to<classfile::OMClassConstantClass>()->nameIndex;
        checkRecursively(file, nid, name, classfile::OMClassConstantType::Utf8);
        break;
    }
    case classfile::OMClassConstantType::String: {
        auto nid = c->to<classfile::OMClassConstantString>()->stringIndex;
        checkRecursively(file, nid, name, classfile::OMClassConstantType::Utf8);
        break;
    }
    case classfile::OMClassConstantType::FieldRef: {
        auto nid = c->to<classfile::OMClassConstantFieldRef>();
        checkRecursively(file, nid->classIndex, name, classfile::OMClassConstantType::Class);
        checkRecursively(file, nid->nameAndTypeIndex, name, classfile::OMClassConstantType::NameAndType);
        break;
    }
    case classfile::OMClassConstantType::MethodRef: {
        auto nid = c->to<classfile::OMClassConstantMethodRef>();
        checkRecursively(file, nid->classIndex, name, classfile::OMClassConstantType::Class);
        checkRecursively(file, nid->nameAndTypeIndex, name, classfile::OMClassConstantType::NameAndType);
        break;
    }
    case classfile::OMClassConstantType::InterfaceMethodRef: {
        auto nid = c->to<classfile::OMClassConstantInterfaceMethodRef>();
        checkRecursively(file, nid->classIndex, name, classfile::OMClassConstantType::Class);
        checkRecursively(file, nid->nameAndTypeIndex, name, classfile::OMClassConstantType::NameAndType);
        break;
    }
    case classfile::OMClassConstantType::NameAndType: {
        auto nid = c->to<classfile::OMClassConstantNameAndType>();
        checkRecursively(file, nid->nameIndex, name, classfile::OMClassConstantType::Utf8);
        checkRecursively(file, nid->descIndex, name, classfile::OMClassConstantType::Utf8);
        break;
    }
    case classfile::OMClassConstantType::MethodHandle: {
        // gino: nothing to check here
        break;
    }
    case classfile::OMClassConstantType::MethodType: {
        auto nid = c->to<classfile::OMClassConstantMethodType>();
        checkRecursively(file, nid->descIndex, name, classfile::OMClassConstantType::Utf8);
        break;
    }
    case classfile::OMClassConstantType::Dynamic: {
        auto nid = c->to<classfile::OMClassConstantDynamic>();
        checkRecursively(file, nid->nameAndTypeIndex, name, classfile::OMClassConstantType::NameAndType);
        break;
    }
    case classfile::OMClassConstantType::InvokeDynamic: {
        auto nid = c->to<classfile::OMClassConstantInvokeDynamic>();
        checkRecursively(file, nid->nameAndTypeIndex, name, classfile::OMClassConstantType::NameAndType);
        break;
    }
    case classfile::OMClassConstantType::Module: {
        auto nid = c->to<classfile::OMClassConstantModule>();
        checkRecursively(file, nid->nameIndex, name, classfile::OMClassConstantType::Utf8);
        break;
    }
    case classfile::OMClassConstantType::Package: {
        auto nid = c->to<classfile::OMClassConstantPackage>();
        checkRecursively(file, nid->nameIndex, name, classfile::OMClassConstantType::Utf8);
        break;
    }
    default:
        throw err::OMValidationError{err::ConstantPool,
                                     fmt::format("unknown constant type {} at #{}", (int)c->type(), id), name};
    }
}
void OMValidator::validateConstantPool(std::shared_ptr<OMClassFile> file, std::string name)
{
    for (auto ic : file->mapping)
    {
        checkRecursively(file, ic.first, name, ic.second->type());
    }
}
std::string OMValidator::fetchContent(int flags)
{
    if (flags & intItem)
    {
        return "(int)";
    }
    if (flags & floatItem)
    {
        return "(float)";
    }
    if (flags & longItem)
    {
        if (flags & slot2Item)
        {
            return "(long, 2nd part)";
        }
        return "(long)";
    }
    if (flags & doubleItem)
    {
        if (flags & slot2Item)
        {
            return "(double, 2nd part)";
        }
        return "(double)";
    }
    if (flags & refItem)
    {
        return "(reference to oop)";
    }

    return "(not initialized)";
}
void OMValidator::safeStackPush(std::stack<int> &stack, OMClassAttrCode *code, std::string pos, int i)
{
    if (stack.size() > code->maxStack)
    {
        throw err::OMValidationError{err::Instructions, "stack depth out of bounds", pos};
    }
    stack.push(i);
}
void OMValidator::safeStackPop(std::stack<int> &stack, OMClassAttrCode *code, std::string pos, int i)
{
    if (stack.size() == 0)
    {
        throw err::OMValidationError{err::Instructions, "stack is empty!", pos};
    }
    if ((stack.top() & i) == 0)
    {
        throw err::OMValidationError{err::Instructions,
                                     fmt::format("stack top type mismatch, required {}, actually {}", fetchContent(i),
                                                 fetchContent(stack.top())),
                                     pos};
    }
    stack.pop();
}
void OMValidator::safeLocalSet(std::vector<int> &local, classfile::OMClassAttrCode *code, std::string pos, int index,
                               int i)
{
    if (index >= local.size())
    {
        throw err::OMValidationError{err::Instructions, "local out of bounds", pos};
    }

    // gino: if we are overriding a 2 slot item ...
    // gino: first, we are overriding the second part of the value
    if (local[index] & slot2Item)
    {
        local[index - 1] = 0x0;
    }
    // gino: otherwise ...
    else if (local[index] & doubleItem || local[index] & longItem)
    {
        local[index + 1] = 0x0;
    }

    local[index] = i;
    if (i == longItem || i == doubleItem)
    {
        local[index + 1] = i | slot2Item;
    }
}
void OMValidator::safeLocalGet(std::vector<int> &local, classfile::OMClassAttrCode *code, std::string pos, int index,
                               int i)
{
    if (index >= local.size())
    {
        throw err::OMValidationError{err::Instructions, "local out of bounds", pos};
    }
    if ((local[index] & i) == 0)
    {
        throw err::OMValidationError{err::Instructions,
                                     fmt::format("local access type mismatch, required {}, actually {}",
                                                 fetchContent(i), fetchContent(local[index])),
                                     pos};
    }
}
int OMValidator::toFlag(std::string name)
{
    switch (hash_compile_time(name.c_str()))
    {
    case "int"_hash:
    case "char"_hash:
    case "short"_hash:
    case "byte"_hash:
    case "boolean"_hash:
        return intItem;
    case "float"_hash:
        return floatItem;
    case "long"_hash:
        return longItem;
    case "double"_hash:
        return doubleItem;
    default:
        return refItem;
    }
}
void OMValidator::safeReturnFetch(std::stack<int> &stack, classfile::OMClassAttrCode *code, std::string pos,
                                  std::string desc)
{
    int i = 0;
    auto fetc = bytecode::descriptor::decodeSignature(desc, &i);
    if (fetc.type == util::Err)
    {
        throw err::OMValidationError{err::Instructions,
                                     fmt::format("unknown descriptor {} ({})", desc, fetc.unwrap_err()), pos};
    }

    stack.push(toFlag(fetc.unwrap().second));
}
void OMValidator::safeArgFetch(std::stack<int> &stack, classfile::OMClassAttrCode *code, std::string pos,
                               std::string desc)
{
    int i = 0;
    auto fetc = bytecode::descriptor::decodeSignature(desc, &i);
    if (fetc.type == util::Err)
    {
        throw err::OMValidationError{err::Instructions,
                                     fmt::format("unknown descriptor {} ({})", desc, fetc.unwrap_err()), pos};
    }

    auto base = fetc.unwrap().first;
    for (auto it = base.rbegin(); it != base.rend(); ++it)
    {
        safeStackPop(stack, code, pos, toFlag(*it));
    }
}
void OMValidator::checkMethod(std::shared_ptr<OMClassFile> file, std::shared_ptr<OMClassMethodInfo> method,
                              std::string name, std::map<int, bool> &checked, OMContext *context, int o)
{
    checkRecursively(file, method->nameIndex, name, OMClassConstantType::Utf8);
    checkRecursively(file, method->descIndex, name, OMClassConstantType::Utf8);

    OMClassAttrCode *code;
    for (auto a : method->attrs)
    {
        if (a->type() == Code)
        {
            code = a->to<OMClassAttrCode>();
        }
    }

    // gino: a normal function without code attribute ?!
    if (code == nullptr && (method->accessFlags & JVM_Acc_Native) == 0 && (method->accessFlags & JVM_Acc_Abstract) == 0)
    {
        throw err::OMValidationError{err::Instructions, "normal function without code attribute!", ""};
    }

    if (method->accessFlags & JVM_Acc_Native || method->accessFlags & JVM_Acc_Abstract)
    {
        return;
    }

    auto mname = file->mapping[method->nameIndex]->to<OMClassConstantUtf8>()->data;
    auto desc = file->mapping[method->descIndex]->to<OMClassConstantUtf8>()->data;

    if (o >= code->codeLength)
    {
        throw err::OMValidationError{err::Instructions, "code offset out of bounds!",
                                     fmt::format("{}.{}{}", name, mname, desc)};
    }

    if (checked.size() == 0)
    {
        for (int i = 0; i < code->codeLength; i++)
        {
            checked[i] = false;
        }
    }

    std::vector<int> locals(code->maxLocals);
    std::stack<int> stack;

    if (context)
    {
        locals = context->locals;
        stack = context->stack;
    }

    auto descInsert = [&](std::string desc, int begin, std::string loc) {
        switch (hash_compile_time(desc.c_str()))
        {
        case "int"_hash:
        case "char"_hash:
        case "byte"_hash:
        case "short"_hash:
        case "boolean"_hash:
            safeLocalSet(locals, code, loc, begin, intItem);
            break;
        case "float"_hash:
            safeLocalSet(locals, code, loc, begin, floatItem);
            break;
        case "long"_hash:
            safeLocalSet(locals, code, loc, begin, longItem);
            break;
        case "double"_hash:
            safeLocalSet(locals, code, loc, begin, doubleItem);
            break;
        default:
            safeLocalSet(locals, code, loc, begin, refItem);
            break;
        }
    };
    auto descPush = [&](std::string desc, std::string loc) {
        switch (hash_compile_time(desc.c_str()))
        {
        case "int"_hash:
        case "char"_hash:
        case "byte"_hash:
        case "short"_hash:
        case "boolean"_hash:
            safeStackPush(stack, code, loc, intItem);
            break;
        case "float"_hash:
            safeStackPush(stack, code, loc, floatItem);
            break;
        case "long"_hash:
            safeStackPush(stack, code, loc, longItem);
            break;
        case "double"_hash:
            safeStackPush(stack, code, loc, doubleItem);
            break;
        default:
            safeStackPush(stack, code, loc, refItem);
            break;
        }
    };

    int begin = 0;
    auto pars = bytecode::descriptor::decodeSignature(desc, &begin);
    if (pars.type == util::Err)
    {
        throw err::OMValidationError{err::Instructions,
                                     fmt::format("unrecognized function descriptor: {} {}", desc, pars.unwrap_err()),
                                     fmt::format("{}.{}{}", name, mname, desc)};
    }
    begin = 0;

    if ((method->accessFlags & JVM_Acc_Static) == 0)
    {
        locals[0] = refItem;
        begin++;
    }
    for (auto refs : pars.unwrap().first)
    {
        descInsert(refs, begin, fmt::format("{}.{}{}", name, mname, desc));
        begin++;
        if (refs == "long" || refs == "double")
        {
            begin++;
        }
    }

    for (int offset = o; offset < code->codeLength;)
    {
        // geopelia: loop or control flow merging, checking complete
        if (checked[offset])
        {
            return;
        }

        auto fn = [&]() { return fmt::format("{}.{}{} + {}", name, mname, desc, offset); };
        auto bump = [&](int s) {
            for (int i = 0; i < s; i++)
            {
                checked[offset + i] = true;
            }
            offset += s;
        };

        switch (code->code->at(offset))
        {
        case op_nop:
            bump(1);
            break;
        case op_aconst_null:
            safeStackPush(stack, code, fn(), refItem);
            bump(1);
            break;
        case op_iconst_i(-1):
        case op_iconst_i(0):
        case op_iconst_i(1):
        case op_iconst_i(2):
        case op_iconst_i(3):
        case op_iconst_i(4):
            safeStackPush(stack, code, fn(), intItem);
            bump(1);
            break;
        case op_lconst_l(0):
        case op_lconst_l(1):
            safeStackPush(stack, code, fn(), longItem);
            bump(1);
            break;
        case op_fconst_f(0):
        case op_fconst_f(1):
        case op_fconst_f(2):
            safeStackPush(stack, code, fn(), floatItem);
            bump(1);
            break;
        case op_dconst_d(0):
            safeStackPush(stack, code, fn(), doubleItem);
            bump(1);
        case op_bipush:
            safeStackPush(stack, code, fn(), intItem);
            bump(2);
            break;
        case op_sipush:
            safeStackPush(stack, code, fn(), intItem);
            bump(3);
            break;
        case op_ldc:
            switch (file->mapping[code->code->at(offset + 1)]->type())
            {
            case OMClassConstantType::Integer:
                safeStackPush(stack, code, fn(), intItem);
                break;
            case OMClassConstantType::Float:
                safeStackPush(stack, code, fn(), floatItem);
                break;
            case OMClassConstantType::String:
                safeStackPush(stack, code, fn(), refItem);
                break;
            default:
                throw err::OMValidationError{err::Instructions, "loading constant with wrong type", fn()};
            }
            bump(2);
            break;

        case op_ldc_w:
            switch (file->mapping[binary::be16ToNative(*(uint16_t *)(code->code->data() + offset))]->type())
            {
            case OMClassConstantType::Integer:
                safeStackPush(stack, code, fn(), intItem);
                break;
            case OMClassConstantType::Float:
                safeStackPush(stack, code, fn(), floatItem);
                break;
            case OMClassConstantType::String:
                safeStackPush(stack, code, fn(), refItem);
                break;
            default:
                throw err::OMValidationError{err::Instructions, "loading constant with wrong type", fn()};
            }
            bump(3);
            break;

        case op_ldc2_w:
            switch (file->mapping[binary::be16ToNative(*(uint16_t *)(code->code->data() + offset))]->type())
            {
            case OMClassConstantType::Long:
                safeStackPush(stack, code, fn(), longItem);
                break;
            case OMClassConstantType::Double:
                safeStackPush(stack, code, fn(), doubleItem);
                break;
            default:
                throw err::OMValidationError{err::Instructions, "loading constant with wrong type", fn()};
            }
            bump(3);
            break;

#define loadOp(op, id)                                                                                                 \
    case op: {                                                                                                         \
        safeLocalGet(locals, code, fn(), code->code->at(offset + 1), id);                                              \
        safeStackPush(stack, code, fn(), intItem);                                                                     \
        bump(2);                                                                                                       \
        break;                                                                                                         \
    }

            loadOp(op_iload, intItem);
            loadOp(op_lload, longItem);
            loadOp(op_fload, floatItem);
            loadOp(op_dload, doubleItem);
            loadOp(op_aload, refItem);

#define loadOpN(op, id)                                                                                                \
    case op(0):                                                                                                        \
    case op(1):                                                                                                        \
    case op(2):                                                                                                        \
    case op(3): {                                                                                                      \
        safeLocalGet(locals, code, fn(), (int)(code->code->at(offset) - op(0)), id);                                   \
        safeStackPush(stack, code, fn(), id);                                                                          \
        bump(1);                                                                                                       \
        break;                                                                                                         \
    }

            loadOpN(op_iload_n, intItem);
            loadOpN(op_lload_n, longItem);
            loadOpN(op_fload_n, floatItem);
            loadOpN(op_dload_n, doubleItem);
            loadOpN(op_aload_n, refItem);

        case op_lcmp: {
            safeStackPop(stack, code, fn(), longItem);
            safeStackPop(stack, code, fn(), longItem);
            safeStackPush(stack, code, fn(), intItem);
            bump(1);
            break;
        }

        // geopelia: check the branch after jumps
        case op_ifge: {
            safeStackPop(stack, code, fn(), intItem);
            auto target = offset + binary::be16SignedToNative(code->code->at(offset + 1), code->code->at(offset + 2));
            // geopelia: copies the current context
            OMContext con{locals, stack};
            checkMethod(file, method, name, checked, &con, target);
            bump(3);
            break;
        }

        case op_return: {
            return;
        }

        case op_getstatic: {
            auto id = binary::be16ToNative(*(uint16_t *)(code->code->data() + offset + 1));
            checkRecursively(file, id, name, OMClassConstantType::FieldRef);
            auto ref = file->mapping[id]->to<OMClassConstantFieldRef>();
            auto nat = file->mapping[ref->nameAndTypeIndex]->to<OMClassConstantNameAndType>();
            auto desc = file->mapping[nat->descIndex]->to<OMClassConstantUtf8>()->data;

            int i = 0;
            auto r = bytecode::descriptor::decodeType(desc, &i);
            if (r.type == Err)
            {
                throw err::OMValidationError{err::Instructions,
                                             fmt::format("unknown type {} ({})", desc, r.unwrap_err()), fn()};
            }

            safeStackPush(stack, code, fn(), toFlag(r.unwrap()));

            bump(3);
            break;
        }

        case op_invokespecial: {
            auto id = binary::be16ToNative(*(uint16_t *)(code->code->data() + offset + 1));
            checkRecursively(file, id, name, OMClassConstantType::MethodRef);
            auto ref = file->mapping[id]->to<OMClassConstantMethodRef>();
            auto nat = file->mapping[ref->nameAndTypeIndex]->to<OMClassConstantNameAndType>();
            auto name = file->mapping[nat->nameIndex]->to<OMClassConstantUtf8>()->data;
            auto desc = file->mapping[nat->descIndex]->to<OMClassConstantUtf8>()->data;
            // gino: calling non constructor is not allowed
            if (name != "<init>")
            {
                throw err::OMValidationError{err::Instructions, "calling non constructor method!", fn()};
            }

            safeArgFetch(stack, code, fn(), desc);

            bump(3);
            break;
        }

        case op_invokevirtual: {
            auto id = binary::be16ToNative(*(uint16_t *)(code->code->data() + offset + 1));
            checkRecursively(file, id, name, OMClassConstantType::MethodRef);
            auto ref = file->mapping[id]->to<OMClassConstantMethodRef>();
            auto nat = file->mapping[ref->nameAndTypeIndex]->to<OMClassConstantNameAndType>();
            auto name = file->mapping[nat->nameIndex]->to<OMClassConstantUtf8>()->data;
            auto desc = file->mapping[nat->descIndex]->to<OMClassConstantUtf8>()->data;
            if (name == "<init>" || name == "<clinit>")
            {
                throw err::OMValidationError{err::Instructions, "calling invalid method!", fn()};
            }

            safeArgFetch(stack, code, fn(), desc);
            safeStackPop(stack, code, fn(), refItem);
            safeReturnFetch(stack, code, fn(), desc);

            bump(3);
            break;
        }

        default:
            logger.info("{} elements in stack", stack.size());
            while (!stack.empty())
            {
                logger.info(fetchContent(stack.top()));
                stack.pop();
            }

            for (int i = 0; i < locals.size(); i++)
            {
                logger.info("#{}: {}", i, fetchContent(locals[i]));
            }
            throw err::OMValidationError{err::Instructions, "unknown instruction!", fn()};
        }
    }
}
} // namespace openminecraft::vm::pixeltower::v3