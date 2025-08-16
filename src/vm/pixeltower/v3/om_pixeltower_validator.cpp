#include "openminecraft/vm/pixeltower/v3/om_pixeltower_validator.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include <memory>
#include <stack>
#include <vector>

using namespace openminecraft::vm::classfile;

constexpr int intItem = 0x0001;
constexpr int floatItem = 0x0002;
constexpr int longItem = 0x0004;
constexpr int doubleItem = 0x0008;
constexpr int charItem = 0x0010;
constexpr int shortItem = 0x0020;
constexpr int byteItem = 0x0040;
// geopelia: maybe the validation may fail before the object initialization
// xxxxxxxx | xxxxxxxx      | xxxxxxxx   | xxxxxxxx    (LE mode)
// (flags)  | (initialized) | (reserved)
constexpr int refItem = 0x0080;
// geopelia: the flag says it's an array
// xxxxxxxx | xxxxxxxx    | xxxxxxxx       | xxxxxxxx    (LE mode)
// (flags)  | (dimension) | (content type) | (reserved)
constexpr int arrItem = 0x0100;

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
        if (flags >> 8)
        {
            return "(reference to oop)";
        }
        else
        {
            return "(reference to oop, not initialized)";
        }
    }
    if (flags & arrItem)
    {
        auto length = flags >> 8 & 0xff;
        auto type = fetchContent(flags >> 16 & 0xff);

        return fmt::format("(array of {} with length {})", type, length);
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

    auto mname = file->mapping[method->nameIndex]->to<OMClassConstantUtf8>()->data;
    auto desc = file->mapping[method->descIndex]->to<OMClassConstantUtf8>()->data;

    std::vector<int> locals(code->maxLocals);
    std::stack<int> stack;

    if ((method->accessFlags & JVM_Acc_Static) == 0)
    {
        // geopelia: local slot 0 refers to this pointer in JVM language sources ...
        locals[0] = refItem;

        if (mname != "<init>")
        {
            locals[0] |= 0b100000000;
        }
    }

    for (int offset = 0; offset < code->codeLength;)
    {
        switch (code->code->at(offset))
        {
        case op_new: {
            stack.push(refItem);
            offset += 3;
            break;
        }
        case op_aload_n(0):
        case op_aload_n(1):
        case op_aload_n(2):
        case op_aload_n(3): {
            stack.push(locals[code->code->at(offset) - op_aload_n(0)]);
            offset++;
            break;
        }
        default: {
            logger.info("{} items in stack", stack.size());
            while (!stack.empty())
            {
                logger.info("{}", fetchContent(stack.top()));
                stack.pop();
            }

            logger.info("{} locals", locals.size());
            for (int i = 0; i < locals.size(); i++)
            {
                logger.info("#{} {}", i, fetchContent(locals[i]));
            }

            throw err::OMValidationError{err::Instructions, "unknown operand detected!",
                                         fmt::format("{}.{}{} + {}", name, mname, desc, offset)};
        }
        }
    }
}
} // namespace openminecraft::vm::pixeltower::v3