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

void OMClassBuilder::klassSuperKlass(const v0::OMKlass *klass)
{
    assert(klass->kind == v0::Normal);
    file->superClass = klassConstantPutClass(klassConstantPutUTF8(klass->name));
}

void OMClassBuilder::klassInterface(const v0::OMKlass *klass)
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
    currentStackHeight = 0;
    maxStackHeight = 0;
    maxLocals = (result->accessFlags & JVM_Acc_Static) ? 0 : 1;
    code = std::make_shared<classfile::OMClassAttrCode>(
        0, 0, 0, std::make_shared<std::vector<uint8_t>>(), 0,
        std::vector<classfile::OMClassAttrCodeExcTable>(), 0, std::vector<std::shared_ptr<classfile::OMClassAttr>>());
}
void OMMethodBuilder::instReturn() const
{
    code->code->push_back(op_return);
}
void OMMethodBuilder::instNop() const
{
    code->code->push_back(op_nop);
}
void OMMethodBuilder::instConst()
{
    code->code->push_back(op_aconst_null);
    codeStackPush();
}
void OMMethodBuilder::instConst(v0::jint i)
{
    if (-1 <= i && i <= 5)
    {
        code->code->push_back(op_iconst_i(i));
    }
    else
    {
        auto id = builder->klassConstantPutNumber<v0::jint, classfile::OMClassConstantInteger,
                                                  classfile::OMClassConstantType::Integer>(i);
        codePutConstantLoading(id);
        codePutVaryId(id);
    }
    codeStackPush();
}
void OMMethodBuilder::instConst(v0::jfloat i)
{
    if (0.f <= i && i <= 2.f)
    {
        code->code->push_back(op_fconst_f(i));
    }
    else
    {
        auto id = builder->klassConstantPutNumber<v0::jfloat, classfile::OMClassConstantFloat,
                                                  classfile::OMClassConstantType::Float>(i);
        codePutConstantLoading(id);
        codePutVaryId(id);
    }
    codeStackPush();
}
void OMMethodBuilder::instConst(v0::jlong i)
{
    if (0 <= i && i <= 1)
    {
        code->code->push_back(op_lconst_l(i));
    }
    else
    {
        code->code->push_back(op_ldc2_w);
        codePutId(builder->klassConstantPutNumber<v0::jlong, classfile::OMClassConstantLong,
                                                  classfile::OMClassConstantType::Long>(i));
    }
    codeStackPush();
}
void OMMethodBuilder::instConst(v0::jdouble i)
{
    if (0.0 <= i && i <= 1.0)
    {
        code->code->push_back(op_dconst_d(i));
    }
    else
    {
        code->code->push_back(op_ldc2_w);
        codePutId(builder->klassConstantPutNumber<v0::jdouble, classfile::OMClassConstantDouble,
                                                  classfile::OMClassConstantType::Double>(i));
    }
    codeStackPush();
}
void OMMethodBuilder::instILoad(uint8_t i)
{
    code->code->push_back(op_iload_n(i));
    codeLocalAccess(i);
    codeStackPush();
}
void OMMethodBuilder::instLLoad(uint8_t i)
{
    code->code->push_back(op_lload_n(i));
    codeLocalAccess(i);
    codeStackPush();
}
void OMMethodBuilder::instFLoad(uint8_t i)
{
    code->code->push_back(op_fload_n(i));
    codeLocalAccess(i);
    codeStackPush();
}
void OMMethodBuilder::instDLoad(uint8_t i)
{
    code->code->push_back(op_dload_n(i));
    codeLocalAccess(i);
    codeStackPush();
}
void OMMethodBuilder::instALoad(uint8_t i)
{
    code->code->push_back(op_aload_n(i));
    codeLocalAccess(i);
    codeStackPush();
}
void OMMethodBuilder::methodCodeFinish() const
{
    code->codeLength = code->code->size();
    code->maxStack = maxStackHeight;
    code->maxLocals = maxLocals;
    result->attrs.push_back(code);
}
} // namespace openminecraft::vm::pixeltower::v3