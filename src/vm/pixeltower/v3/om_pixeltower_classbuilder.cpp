#include <utility>

#include "openminecraft/vm/pixeltower/v3/om_pixeltower_classbuilder.hpp"

namespace openminecraft::vm::pixeltower::v3
{
OMClassBuilder::OMClassBuilder() : logger("OMClassBuilder", this)
{
}

void OMClassBuilder::klassBegin()
{
    file = std::make_shared<classfile::OMClassFile>();
    file->constants.push_back(nullptr);
}

void OMClassBuilder::klassVersion(int major, int minor) const
{
    file->magicNumber = 0xCAFEBABE;
    file->major = major;
    file->minor = minor;
}

void OMClassBuilder::klassAccessFlags(int f) const
{
    file->accessFlags = f;
}

uint16_t OMClassBuilder::klassConstantPut(const std::shared_ptr<classfile::OMClassConstant> &constant) const
{
    auto id = file->constants.size();
    file->constants.push_back(constant);
    file->mapping[id] = constant;
    if (constant->type() == classfile::OMClassConstantType::Long ||
        constant->type() == classfile::OMClassConstantType::Double)
    {
        // geopelia: we need to reserve 1 more slot for long/double constants
        file->constants.push_back(nullptr);
    }
    return id;
}

void OMClassBuilder::klassName(const std::string &name)
{
    file->thisClass = klassConstantPutClass(klassConstantPutUTF8(name));
}

void OMClassBuilder::klassSuperKlass(v0::OMKlass *klass)
{
    assert(klass->kind == v0::Normal);
    file->superClass = klassConstantPutClass(klassConstantPutUTF8(klass->name));
}

void OMClassBuilder::klassInterface(v0::OMKlass *klass)
{
    assert(klass->kind == v0::Interface);
    file->interfaces.push_back(klassConstantPutClass(klassConstantPutUTF8(klass->name)));
}

std::shared_ptr<OMMethodBuilder> OMClassBuilder::klassConstructMethod()
{
    return std::make_shared<OMMethodBuilder>(this);
}

OMMethodBuilder::OMMethodBuilder(OMClassBuilder *builder) : builder(builder)
{
}

void OMMethodBuilder::methodBegin()
{
    result = std::make_shared<classfile::OMClassMethodInfo>();
}

void OMMethodBuilder::methodNameAndDesc(std::string s, std::string desc) const
{
    result->nameIndex = builder->klassConstantPutUTF8(std::move(s));
    result->descIndex = builder->klassConstantPutUTF8(std::move(desc));
}
void OMMethodBuilder::methodAccessFlags(int f) const
{
    result->accessFlags = f;
}
void OMMethodBuilder::methodFinish() const
{
    builder->file->methods.push_back(result);
}
void OMMethodBuilder::methodCodeBegin()
{
    // code = std::make_shared<classfile::OMClassAttrCode>();
}
void OMMethodBuilder::methodCodeFinish() const
{
    result->attrs.push_back(code);
}
} // namespace openminecraft::vm::pixeltower::v3