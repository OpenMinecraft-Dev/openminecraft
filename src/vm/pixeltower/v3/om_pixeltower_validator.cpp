#include "openminecraft/vm/pixeltower/v3/om_pixeltower_validator.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include <cstdint>
#include <memory>
#include <stack>
#include <unordered_map>
#include <vector>

using namespace openminecraft::vm::classfile;

constexpr int intItem = 0x0001;
constexpr int floatItem = 0x0002;
constexpr int longItem = 0x0004;
constexpr int doubleItem = 0x0008;
constexpr int charItem = 0x0010;
constexpr int shortItem = 0x0020;
constexpr int byteItem = 0x0040;
// geopelia: if it's a reference of an oop, check the 2nd segment of the flag
// xxxxxxx | x ...
// (flags) | (initialized)
// geopelia: if it is an array
// xxxxxxxx | xxxxxxxx    | xxxxxxxx       | xxxxxxxx    (LE mode)
// (flags)  | (dimension) | (content type) | (reserved)
constexpr int refItem = 0x0080;

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
        checkMethod(file, m, name);
    }
}
void OMValidator::checkRecursively(std::shared_ptr<OMClassFile> file, uint16_t id, std::string name,
                                   OMClassConstantType type)
{
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
        return "(long)";
    }
    if (flags & doubleItem)
    {
        return "(double)";
    }
    if (flags & charItem)
    {
        return "(char)";
    }
    if (flags & shortItem)
    {
        return "(short)";
    }
    if (flags & byteItem)
    {
        return "(byte)";
    }
    if (flags & refItem)
    {
        int dim = flags >> 8 & 0xFF;
        if (dim)
        {
            auto type = fetchContent(flags >> 16 & 0xff);

            return fmt::format("(array of {} with dimension {})", type, dim);
        }

        return "(reference to oop)";
    }

    return "(not initialized)";
}
void OMValidator::checkMethod(std::shared_ptr<OMClassFile> file, std::shared_ptr<OMClassMethodInfo> method,
                              std::string name)
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

    std::vector<int> locals(code->maxLocals);
    std::stack<int> stack;

    if ((method->accessFlags & JVM_Acc_Static) == 0)
    {
        // geopelia: local slot 0 refers to **this** pointer in JVM language sources ...
        locals[0] = refItem;
    }

    std::unordered_map<int, int> jumps;
    for (int offset = 0; offset < code->codeLength;)
    {
        switch (code->code->at(offset))
        {
        case op_nop:
        case op_aconst_null:
        case op_iconst_i(-1):
        case op_iconst_i(0):
        case op_iconst_i(1):
        case op_iconst_i(2):
        case op_iconst_i(3):
        case op_iconst_i(4):
        case op_iconst_i(5):
        case op_lconst_l(0):
        case op_lconst_l(1):
        case op_fconst_f(0):
        case op_fconst_f(1):
        case op_dconst_d(0):
        case op_dconst_d(1):
        case op_iload_n(0):
        case op_iload_n(1):
        case op_iload_n(2):
        case op_iload_n(3):
        case op_lload_n(0):
        case op_lload_n(1):
        case op_lload_n(2):
        case op_lload_n(3):
        case op_fload_n(0):
        case op_fload_n(1):
        case op_fload_n(2):
        case op_fload_n(3):
        case op_dload_n(0):
        case op_dload_n(1):
        case op_dload_n(2):
        case op_dload_n(3):
        case op_aload_n(0):
        case op_aload_n(1):
        case op_aload_n(2):
        case op_aload_n(3):
        case op_istore_n(0):
        case op_istore_n(1):
        case op_istore_n(2):
        case op_istore_n(3):
        case op_lstore_n(0):
        case op_lstore_n(1):
        case op_lstore_n(2):
        case op_lstore_n(3):
        case op_fstore_n(0):
        case op_fstore_n(1):
        case op_fstore_n(2):
        case op_fstore_n(3):
        case op_dstore_n(0):
        case op_dstore_n(1):
        case op_dstore_n(2):
        case op_dstore_n(3):
        case op_astore_n(0):
        case op_astore_n(1):
        case op_astore_n(2):
        case op_astore_n(3):
        case op_iaload:
        case op_laload:
        case op_faload:
        case op_daload:
        case op_aaload:
        case op_baload:
        case op_caload:
        case op_saload:
        case op_iastore:
        case op_lastore:
        case op_fastore:
        case op_dastore:
        case op_aastore:
        case op_bastore:
        case op_castore:
        case op_sastore:
        case op_pop:
        case op_pop2:
        case op_dup:
        case op_dup_x1:
        case op_dup_x2:
        case op_dup2:
        case op_dup2_x1:
        case op_dup2_x2:
        case op_swap:
        case op_iadd:
        case op_ladd:
        case op_fadd:
        case op_dadd:
        case op_isub:
        case op_lsub:
        case op_fsub:
        case op_dsub:
        case op_imul:
        case op_lmul:
        case op_fmul:
        case op_dmul:
        case op_idiv:
        case op_ldiv:
        case op_fdiv:
        case op_ddiv:
        case op_irem:
        case op_lrem:
        case op_frem:
        case op_drem:
        case op_ineg:
        case op_lneg:
        case op_fneg:
        case op_dneg:
        case op_ishl:
        case op_lshl:
        case op_ishr:
        case op_lshr:
        case op_iushr:
        case op_lushr:
        case op_iand:
        case op_land:
        case op_ior:
        case op_lor:
        case op_ixor:
        case op_lxor:
        case op_i2l:
        case op_i2f:
        case op_i2d:
        case op_l2i:
        case op_l2f:
        case op_l2d:
        case op_f2i:
        case op_f2l:
        case op_d2i:
        case op_f2d:
        case op_d2l:
        case op_d2f:
        case op_i2b:
        case op_i2c:
        case op_i2s:
        case op_lcmp:
        case op_fcmpl:
        case op_fcmpg:
        case op_dcmpl:
        case op_dcmpg:
        case op_arraylength:
        case op_athrow:
        case op_monitorenter:
        case op_monitorexit:
            offset++;
            break;
        case op_bipush:
        case op_ldc:
        case op_iload:
        case op_lload:
        case op_fload:
        case op_dload:
        case op_aload:
        case op_istore:
        case op_lstore:
        case op_fstore:
        case op_dstore:
        case op_astore:
        case op_newarray:
            offset += 2;
            break;
        case op_sipush:
        case op_ldc_w:
        case op_ldc2_w:
        case op_iinc:
        case op_getstatic:
        case op_putstatic:
        case op_getfield:
        case op_putfield:
        case op_invokevirtual:
        case op_invokespecial:
        case op_invokestatic:
        case op_invokeinterface:
        case op_invokedynamic:
        case op_new:
        case op_anewarray:
        case op_checkcast:
        case op_instanceof:
            offset += 3;
            break;
        case op_multianewarray:
            offset += 4;
            break;
        case op_ifeq:
        case op_ifne:
        case op_iflt:
        case op_ifge:
        case op_ifgt:
        case op_ifle:
        case op_if_icmpeq:
        case op_if_icmpne:
        case op_if_icmplt:
        case op_if_icmpge:
        case op_if_icmpgt:
        case op_if_icmple:
        case op_if_acmpeq:
        case op_if_acmpne:
        case op_goto:
        case op_jsr:
        case op_ifnull:
        case op_ifnonnull: {
            // gino: another code block!
            auto target = offset + binary::be16SignedToNative(code->code->at(offset + 1), code->code->at(offset + 2));
            jumps[offset] = target;
            offset += 3;
            break;
        }
        case op_ret:
            offset += 2;
            break;
        case op_goto_w:
        case op_jsr_w: {
            auto target = offset + binary::be32SignedToNative(code->code->at(offset + 1), code->code->at(offset + 2),
                                                              code->code->at(offset + 3), code->code->at(offset + 4));
            jumps[offset] = target;
            offset += 5;
            break;
        }
        case op_tableswitch:
        case op_lookupswitch:
            logger.info("{}", offset);
            throw 0;
            // gino: complex structures inside the operand
            break;
        case op_ireturn:
        case op_lreturn:
        case op_freturn:
        case op_dreturn:
        case op_areturn:
        case op_return:
            // gino: ends a code block
            jumps[offset] = -1;
            offset++;
            break;
        case op_wide:
            if (code->code->at(offset + 1) == op_iinc)
            {
                offset += 4;
                break;
            }
            else
            {
                offset += 6;
                break;
            }
            break;
        default:
            break;
        }
    }

    logger.info("{}.{}{}", name, mname, desc);
    for (auto &p : jumps)
    {
        logger.info("{} -> {}", p.first, p.second);
    }
    logger.info("-----------");
}
} // namespace openminecraft::vm::pixeltower::v3