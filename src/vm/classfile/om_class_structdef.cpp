#include <openminecraft/vm/classfile/om_class_file.hpp>
#include <utility>
#include <vector>

namespace openminecraft::vm::classfile
{
OMClassConstant::~OMClassConstant() = default;
OMClassAttr::~OMClassAttr() = default;
OMClassAttrVerifyStackMapFrame::OMClassAttrVerifyStackMapFrame() : tag(0)
{
}
OMClassAttrVerifyStackMapFrame::~OMClassAttrVerifyStackMapFrame()
{
}
OMClassConstantFieldRef::OMClassConstantFieldRef(uint16_t ci, uint16_t nti) : classIndex(ci), nameAndTypeIndex(nti)
{
}
OMClassConstantType OMClassConstantFieldRef::type()
{
    return OMClassConstantType::FieldRef;
}
OMClassConstantMethodRef::OMClassConstantMethodRef(uint16_t ci, uint16_t nti) : classIndex(ci), nameAndTypeIndex(nti)
{
}
OMClassConstantType OMClassConstantMethodRef::type()
{
    return OMClassConstantType::MethodRef;
}
OMClassConstantInterfaceMethodRef::OMClassConstantInterfaceMethodRef(uint16_t ci, uint16_t nti)
    : classIndex(ci), nameAndTypeIndex(nti)
{
}
OMClassConstantType OMClassConstantInterfaceMethodRef::type()
{
    return OMClassConstantType::InterfaceMethodRef;
}
OMClassConstantClass::OMClassConstantClass(uint16_t ni) : nameIndex(ni)
{
}
OMClassConstantType OMClassConstantClass::type()
{
    return OMClassConstantType::Class;
}
OMClassConstantNameAndType::OMClassConstantNameAndType(uint16_t ni, uint16_t di) : nameIndex(ni), descIndex(di)
{
}
OMClassConstantType OMClassConstantNameAndType::type()
{
    return OMClassConstantType::NameAndType;
}
OMClassConstantUtf8::OMClassConstantUtf8(std::string data) : data(std::move(data))
{
}
OMClassConstantType OMClassConstantUtf8::type()
{
    return OMClassConstantType::Utf8;
}
OMClassConstantString::OMClassConstantString(uint16_t si) : stringIndex(si)
{
}
OMClassConstantType OMClassConstantString::type()
{
    return OMClassConstantType::String;
}
OMClassConstantInteger::OMClassConstantInteger(int data) : data(data)
{
}
OMClassConstantType OMClassConstantInteger::type()
{
    return OMClassConstantType::Integer;
}
OMClassConstantFloat::OMClassConstantFloat(float data) : data(data)
{
}
OMClassConstantType OMClassConstantFloat::type()
{
    return OMClassConstantType::Float;
}
OMClassConstantLong::OMClassConstantLong(int64_t data) : data(data)
{
}
OMClassConstantType OMClassConstantLong::type()
{
    return OMClassConstantType::Long;
}
OMClassConstantDouble::OMClassConstantDouble(double data) : data(data)
{
}
OMClassConstantType OMClassConstantDouble::type()
{
    return OMClassConstantType::Double;
}
OMClassConstantMethodHandle::OMClassConstantMethodHandle(uint8_t rk, uint16_t ri) : refKind(rk), refIndex(ri)
{
}
OMClassConstantType OMClassConstantMethodHandle::type()
{
    return OMClassConstantType::MethodHandle;
}
OMClassConstantMethodType::OMClassConstantMethodType(uint16_t di) : descIndex(di)
{
}
OMClassConstantType OMClassConstantMethodType::type()
{
    return OMClassConstantType::MethodType;
}
OMClassConstantDynamic::OMClassConstantDynamic(uint16_t bmai, uint16_t nti)
    : bootstrapMethodAttrIndex(bmai), nameAndTypeIndex(nti)
{
}
OMClassConstantType OMClassConstantDynamic::type()
{
    return OMClassConstantType::Dynamic;
}
OMClassConstantInvokeDynamic::OMClassConstantInvokeDynamic(uint16_t bmai, uint16_t nti)
    : bootstrapMethodAttrIndex(bmai), nameAndTypeIndex(nti)
{
}
OMClassConstantType OMClassConstantInvokeDynamic::type()
{
    return OMClassConstantType::InvokeDynamic;
}
OMClassConstantModule::OMClassConstantModule(uint16_t ni) : nameIndex(ni)
{
}
OMClassConstantType OMClassConstantModule::type()
{
    return OMClassConstantType::Module;
}
OMClassConstantPackage::OMClassConstantPackage(uint16_t ni) : nameIndex(ni)
{
}
OMClassConstantType OMClassConstantPackage::type()
{
    return OMClassConstantType::Package;
}

OMClassAttrConstantValue::OMClassAttrConstantValue(uint16_t vi) : valueIndex(vi)
{
}
OMClassAttrType OMClassAttrConstantValue::type()
{
    return OMClassAttrType::ConstantValue;
}

OMClassAttrCode::OMClassAttrCode(uint16_t ms, uint16_t ml, uint32_t cl, std::shared_ptr<std::vector<uint8_t>> c,
                                 uint16_t etl, std::vector<OMClassAttrCodeExcTable> et, uint16_t ac,
                                 std::vector<std::shared_ptr<OMClassAttr>> a)
    : maxStack(ms), maxLocals(ml), codeLength(cl), code(std::move(c)), excTableLength(etl), excTable(std::move(et)),
      attributesCount(ac), attributes(std::move(a))
{
}
OMClassAttrType OMClassAttrCode::type()
{
    return OMClassAttrType::Code;
}

OMClassAttrStackMapTable::OMClassAttrStackMapTable(uint16_t noe,
                                                   std::vector<std::shared_ptr<OMClassAttrVerifyStackMapFrame>> e)
    : numberOfEntries(noe), entries(std::move(e))
{
}
OMClassAttrType OMClassAttrStackMapTable::type()
{
    return OMClassAttrType::StackMapTable;
}

OMClassAttrExceptions::OMClassAttrExceptions(uint16_t noe, std::vector<uint16_t> eit)
    : numberOfExceptions(noe), exceptionIndexTable(std::move(eit))
{
}
OMClassAttrType OMClassAttrExceptions::type()
{
    return OMClassAttrType::Exceptions;
}

OMClassAttrInnerClass::OMClassAttrInnerClass(uint16_t numberOfClasses, std::vector<OMClassAttrInnerClassInfo> classes)
    : numberOfClasses(numberOfClasses), classes(std::move(classes))
{
}
OMClassAttrType OMClassAttrInnerClass::type()
{
    return OMClassAttrType::InnerClasses;
}

OMClassAttrEnclosingMethod::OMClassAttrEnclosingMethod(uint16_t ci, uint16_t mi) : classIndex(ci), methodIndex(mi)
{
}
OMClassAttrType OMClassAttrEnclosingMethod::type()
{
    return OMClassAttrType::EnclosingMethod;
}

OMClassAttrSynthetic::OMClassAttrSynthetic() = default;
OMClassAttrType OMClassAttrSynthetic::type()
{
    return OMClassAttrType::Synthetic;
}

OMClassAttrSignature::OMClassAttrSignature(uint16_t si) : signatureIndex(si)
{
}
OMClassAttrType OMClassAttrSignature::type()
{
    return OMClassAttrType::Signature;
}

OMClassAttrSourceFile::OMClassAttrSourceFile(uint16_t si) : sourcefileIndex(si)
{
}
OMClassAttrType OMClassAttrSourceFile::type()
{
    return OMClassAttrType::SourceFile;
}

OMClassAttrSourceDebugExtension::OMClassAttrSourceDebugExtension(std::vector<uint8_t> de) : debugExt(std::move(de))
{
}
OMClassAttrType OMClassAttrSourceDebugExtension::type()
{
    return OMClassAttrType::SourceDebugExtension;
}

OMClassAttrLineNumberTable::OMClassAttrLineNumberTable(uint16_t lntl, std::unordered_map<uint16_t, uint16_t> lnt)
    : lineNumberTableLength(lntl), lineNumberTable(std::move(lnt))
{
}
OMClassAttrType OMClassAttrLineNumberTable::type()
{
    return OMClassAttrType::LineNumberTable;
}

OMClassAttrLocalVarTable::OMClassAttrLocalVarTable(uint16_t lvtl, std::vector<OMClassAttrLocalVar> lvt)
    : localVarTableLength(lvtl), localVarTable(std::move(lvt))
{
}
OMClassAttrType OMClassAttrLocalVarTable::type()
{
    return OMClassAttrType::LocalVariableTable;
}

OMClassAttrLocalVarTypeTable::OMClassAttrLocalVarTypeTable(uint16_t lvtl, std::vector<OMClassAttrLocalVar> lvt)
    : localVarTableLength(lvtl), localVarTable(std::move(lvt))
{
}
OMClassAttrType OMClassAttrLocalVarTypeTable::type()
{
    return OMClassAttrType::LocalVariableTypeTable;
}

OMClassAttrDeprecated::OMClassAttrDeprecated() = default;
OMClassAttrType OMClassAttrDeprecated::type()
{
    return OMClassAttrType::Deprecated;
}

OMClassAttrNestHost::OMClassAttrNestHost(uint16_t hci) : hostClassIndex(hci)
{
}
OMClassAttrType OMClassAttrNestHost::type()
{
    return OMClassAttrType::NestHost;
}

OMClassAttrNestMembers::OMClassAttrNestMembers(uint16_t noc, std::vector<uint16_t> classes)
    : numberOfClasses(noc), classes(std::move(classes))
{
}
OMClassAttrType OMClassAttrNestMembers::type()
{
    return OMClassAttrType::NestMembers;
}

OMClassAttrPermittedSubclasses::OMClassAttrPermittedSubclasses(uint16_t noc, std::vector<uint16_t> classes)
    : numberOfClasses(noc), classes(std::move(classes))
{
}
OMClassAttrType OMClassAttrPermittedSubclasses::type()
{
    return OMClassAttrType::PermittedSubclasses;
}

OMClassAttrModuleMainClass::OMClassAttrModuleMainClass(uint16_t mci) : mainClassIndex(mci)
{
}
OMClassAttrType OMClassAttrModuleMainClass::type()
{
    return OMClassAttrType::ModuleMainClass;
}

OMClassAttrModulePackages::OMClassAttrModulePackages(uint16_t pc, std::vector<uint16_t> pi)
    : packageCount(pc), packageIndex(std::move(pi))
{
}
OMClassAttrType OMClassAttrModulePackages::type()
{
    return OMClassAttrType::ModulePackages;
}

OMClassAttrRuntimeVisibleAnnotations::OMClassAttrRuntimeVisibleAnnotations(
    uint16_t na, std::vector<std::shared_ptr<OMClassAnnotation>> data)
    : numAnnotations(na), annotations(std::move(data))
{
}

OMClassAttrType OMClassAttrRuntimeVisibleAnnotations::type()
{
    return OMClassAttrType::RuntimeVisibleAnnotations;
}

OMClassAttrRuntimeInvisibleAnnotations::OMClassAttrRuntimeInvisibleAnnotations(
    uint16_t na, std::vector<std::shared_ptr<OMClassAnnotation>> data)
    : numAnnotations(na), annotations(std::move(data))
{
}

OMClassAttrType OMClassAttrRuntimeInvisibleAnnotations::type()
{
    return OMClassAttrType::RuntimeInvisibleAnnotations;
}

OMClassAttrAnnotationDefault::OMClassAttrAnnotationDefault(std::shared_ptr<OMClassAnnotationElemValue> v)
    : value(std::move(v))
{
}

OMClassAttrType OMClassAttrAnnotationDefault::type()
{
    return OMClassAttrType::AnnotationDefault;
}

OMClassAttrMethodParameters::OMClassAttrMethodParameters(uint8_t pc, std::vector<OMClassParam> p)
    : paramCount(pc), params(std::move(p))
{
}
OMClassAttrType OMClassAttrMethodParameters::type()
{
    return OMClassAttrType::MethodParameters;
}

OMClassAttrBootMethods::OMClassAttrBootMethods(uint16_t n, std::vector<OMClassBootMethods> d)
    : numBootMethods(n), bootMethods(std::move(d))
{
}
OMClassAttrType OMClassAttrBootMethods::type()
{
    return OMClassAttrType::BootstrapMethods;
}

OMClassAttrRecord::OMClassAttrRecord(uint16_t c, std::vector<OMClassRecordCompInfo> i)
    : numComps(c), comps(std::move(i))
{
}
OMClassAttrType OMClassAttrRecord::type()
{
    return OMClassAttrType::Record;
}

OMClassAttrRuntimeVisibleParameterAnnotations::OMClassAttrRuntimeVisibleParameterAnnotations(
    uint8_t n, std::vector<OMClassParamAnnotations> d)
    : numParams(n), params(std::move(d))
{
}
OMClassAttrType OMClassAttrRuntimeVisibleParameterAnnotations::type()
{
    return OMClassAttrType::RuntimeVisibleParameterAnnotations;
}

OMClassAttrRuntimeInvisibleParameterAnnotations::OMClassAttrRuntimeInvisibleParameterAnnotations(
    uint8_t n, std::vector<OMClassParamAnnotations> d)
    : numParams(n), params(std::move(d))
{
}
OMClassAttrType OMClassAttrRuntimeInvisibleParameterAnnotations::type()
{
    return OMClassAttrType::RuntimeInvisibleParameterAnnotations;
}

OMClassRuntimeTypeAnnotationInfo::OMClassRuntimeTypeAnnotationInfo()
{
}

OMClassRuntimeTypeAnnotationInfo::~OMClassRuntimeTypeAnnotationInfo()
{
}

OMClassAttrRuntimeVisibleTypeAnnotation::OMClassAttrRuntimeVisibleTypeAnnotation(uint16_t na,
                                                                         std::vector<std::shared_ptr<OMClassRuntimeTypeAnnotation>> a)
    : numAnnotations(na), annotations(std::move(a))
{
}

OMClassAttrType OMClassAttrRuntimeVisibleTypeAnnotation::type()
{
    return OMClassAttrType::RuntimeVisibleTypeAnnotations;
}

OMClassAttrRuntimeInvisibleTypeAnnotation::OMClassAttrRuntimeInvisibleTypeAnnotation(
    uint16_t na, std::vector<std::shared_ptr<OMClassRuntimeTypeAnnotation>> a)
    : numAnnotations(na), annotations(std::move(a))
{
}

OMClassAttrType OMClassAttrRuntimeInvisibleTypeAnnotation::type()
{
    return OMClassAttrType::RuntimeInvisibleTypeAnnotations;
}

OMClassAttrModule::OMClassAttrModule(uint16_t mni, uint16_t mf, uint16_t mvi, uint16_t rc, std::vector<OMClassModuleRequire> r, uint16_t ec, std::vector<OMClassModuleExport> e, uint16_t oc, std::vector<OMClassModuleOpen> o, uint16_t uc, std::vector<uint16_t> u, uint16_t pc, std::vector<OMClassModuleProvide> p): moduleNameIndex(mni), moduleFlags(mf), moduleVersionIndex(mvi), requiresCount(rc), requires(r), exportsCount(ec), exports(e), opensCount(oc), opens(o), usesCount(uc), usesIndex(u), providesCount(pc), provides(p)
{
}

OMClassAttrType OMClassAttrModule::type()
{
    return OMClassAttrType::Module;
}

} // namespace openminecraft::vm::classfile