#include "openminecraft/vm/pixeltower/v3/om_pixeltower_validator.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include <memory>

namespace openminecraft::vm::pixeltower::v3
{
OMValidator::OMValidator() : logger("OMValidator", this)
{
}

void OMValidator::validate(std::shared_ptr<classfile::OMClassFile> file, std::string name)
{
    validateConstantPool(file, name);
}
void OMValidator::checkRecursively(std::shared_ptr<classfile::OMClassFile> file,
                                   std::shared_ptr<classfile::OMClassConstant> c, uint16_t id, std::string name,
                                   classfile::OMClassConstantType type)
{
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
        checkRecursively(file, file->mapping[nid], nid, name, classfile::OMClassConstantType::Utf8);
        break;
    }
    case classfile::OMClassConstantType::String: {
        auto nid = c->to<classfile::OMClassConstantString>()->stringIndex;
        checkRecursively(file, file->mapping[nid], nid, name, classfile::OMClassConstantType::Utf8);
        break;
    }
    case classfile::OMClassConstantType::FieldRef: {
        auto nid = c->to<classfile::OMClassConstantFieldRef>();
        checkRecursively(file, file->mapping[nid->classIndex], nid->classIndex, name,
                         classfile::OMClassConstantType::Class);
        checkRecursively(file, file->mapping[nid->nameAndTypeIndex], nid->nameAndTypeIndex, name,
                         classfile::OMClassConstantType::NameAndType);
        break;
    }
    case classfile::OMClassConstantType::MethodRef: {
        auto nid = c->to<classfile::OMClassConstantMethodRef>();
        checkRecursively(file, file->mapping[nid->classIndex], nid->classIndex, name,
                         classfile::OMClassConstantType::Class);
        checkRecursively(file, file->mapping[nid->nameAndTypeIndex], nid->nameAndTypeIndex, name,
                         classfile::OMClassConstantType::NameAndType);
        break;
    }
    case classfile::OMClassConstantType::InterfaceMethodRef: {
        auto nid = c->to<classfile::OMClassConstantInterfaceMethodRef>();
        checkRecursively(file, file->mapping[nid->classIndex], nid->classIndex, name,
                         classfile::OMClassConstantType::Class);
        checkRecursively(file, file->mapping[nid->nameAndTypeIndex], nid->nameAndTypeIndex, name,
                         classfile::OMClassConstantType::NameAndType);
        break;
    }
    case classfile::OMClassConstantType::NameAndType: {
        auto nid = c->to<classfile::OMClassConstantNameAndType>();
        checkRecursively(file, file->mapping[nid->nameIndex], nid->nameIndex, name,
                         classfile::OMClassConstantType::Utf8);
        checkRecursively(file, file->mapping[nid->descIndex], nid->descIndex, name,
                         classfile::OMClassConstantType::Utf8);
        break;
    }
    case classfile::OMClassConstantType::MethodHandle: {
        // gino: nothing to check here
        break;
    }
    case classfile::OMClassConstantType::MethodType: {
        auto nid = c->to<classfile::OMClassConstantMethodType>();
        checkRecursively(file, file->mapping[nid->descIndex], nid->descIndex, name,
                         classfile::OMClassConstantType::Utf8);
        break;
    }
    case classfile::OMClassConstantType::Dynamic: {
        auto nid = c->to<classfile::OMClassConstantDynamic>();
        checkRecursively(file, file->mapping[nid->nameAndTypeIndex], nid->nameAndTypeIndex, name,
                         classfile::OMClassConstantType::NameAndType);
        break;
    }
    case classfile::OMClassConstantType::InvokeDynamic: {
        auto nid = c->to<classfile::OMClassConstantInvokeDynamic>();
        checkRecursively(file, file->mapping[nid->nameAndTypeIndex], nid->nameAndTypeIndex, name,
                         classfile::OMClassConstantType::NameAndType);
        break;
    }
    case classfile::OMClassConstantType::Module: {
        auto nid = c->to<classfile::OMClassConstantModule>();
        checkRecursively(file, file->mapping[nid->nameIndex], nid->nameIndex, name,
                         classfile::OMClassConstantType::Utf8);
        break;
    }
    case classfile::OMClassConstantType::Package: {
        auto nid = c->to<classfile::OMClassConstantPackage>();
        checkRecursively(file, file->mapping[nid->nameIndex], nid->nameIndex, name,
                         classfile::OMClassConstantType::Utf8);
        break;
    }
    default:
        throw err::OMValidationError{err::ConstantPool,
                                     fmt::format("unknown constant type {} at #{}", (int)c->type(), id), name};
    }
}
void OMValidator::validateConstantPool(std::shared_ptr<classfile::OMClassFile> file, std::string name)
{

    for (auto ic : file->mapping)
    {
        checkRecursively(file, ic.second, ic.first, name, ic.second->type());
    }
}
} // namespace openminecraft::vm::pixeltower::v3