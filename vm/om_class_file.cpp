#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <cstdint>
#include <fmt/format.h>
#include <openminecraft/binary/om_bin_endians.hpp>
#include <openminecraft/vm/om_class_file.hpp>
#include <stdexcept>
#include <vector>

using namespace openminecraft::binary;
using namespace openminecraft::binary::hash;

namespace openminecraft::vm::classfile {
OMClassConstant::~OMClassConstant() { }
OMClassAttr::~OMClassAttr() { }
OMClassAttrVerifyStackMapFrame::OMClassAttrVerifyStackMapFrame()
    : tag(0)
{
}
OMClassAttrVerifyStackMapFrame::~OMClassAttrVerifyStackMapFrame() { }
OMClassConstantFieldRef::OMClassConstantFieldRef(uint16_t ci, uint16_t nti)
    : classIndex(ci)
    , nameAndTypeIndex(nti)
{
}
OMClassConstantType OMClassConstantFieldRef::type()
{
    return OMClassConstantType::FieldRef;
}
OMClassConstantMethodRef::OMClassConstantMethodRef(uint16_t ci, uint16_t nti)
    : classIndex(ci)
    , nameAndTypeIndex(nti)
{
}
OMClassConstantType OMClassConstantMethodRef::type()
{
    return OMClassConstantType::MethodRef;
}
OMClassConstantInterfaceMethodRef::OMClassConstantInterfaceMethodRef(uint16_t ci, uint16_t nti)
    : classIndex(ci)
    , nameAndTypeIndex(nti)
{
}
OMClassConstantType OMClassConstantInterfaceMethodRef::type()
{
    return OMClassConstantType::InterfaceMethodRef;
}
OMClassConstantClass::OMClassConstantClass(uint16_t ni)
    : nameIndex(ni)
{
}
OMClassConstantType OMClassConstantClass::type()
{
    return OMClassConstantType::Class;
}
OMClassConstantNameAndType::OMClassConstantNameAndType(uint16_t ni, uint16_t di)
    : nameIndex(ni)
    , descIndex(di)
{
}
OMClassConstantType OMClassConstantNameAndType::type()
{
    return OMClassConstantType::NameAndType;
}
OMClassConstantUtf8::OMClassConstantUtf8(std::string data)
    : data(data)
{
}
OMClassConstantType OMClassConstantUtf8::type()
{
    return OMClassConstantType::Utf8;
}
OMClassConstantString::OMClassConstantString(uint16_t si)
    : stringIndex(si)
{
}
OMClassConstantType OMClassConstantString::type()
{
    return OMClassConstantType::String;
}
OMClassConstantInteger::OMClassConstantInteger(int data)
    : data(data)
{
}
OMClassConstantType OMClassConstantInteger::type()
{
    return OMClassConstantType::Integer;
}
OMClassConstantFloat::OMClassConstantFloat(float data)
    : data(data)
{
}
OMClassConstantType OMClassConstantFloat::type()
{
    return OMClassConstantType::Float;
}
OMClassConstantLong::OMClassConstantLong(int64_t data)
    : data(data)
{
}
OMClassConstantType OMClassConstantLong::type()
{
    return OMClassConstantType::Long;
}
OMClassConstantDouble::OMClassConstantDouble(double data)
    : data(data)
{
}
OMClassConstantType OMClassConstantDouble::type()
{
    return OMClassConstantType::Double;
}
OMClassConstantMethodHandle::OMClassConstantMethodHandle(uint8_t rk, uint16_t ri)
    : refKind(rk)
    , refIndex(ri)
{
}
OMClassConstantType OMClassConstantMethodHandle::type()
{
    return OMClassConstantType::MethodHandle;
}
OMClassConstantMethodType::OMClassConstantMethodType(uint16_t di)
    : descIndex(di)
{
}
OMClassConstantType OMClassConstantMethodType::type()
{
    return OMClassConstantType::MethodType;
}
OMClassConstantDynamic::OMClassConstantDynamic(uint16_t bmai, uint16_t nti)
    : bootstrapMethodAttrIndex(bmai)
    , nameAndTypeIndex(nti)
{
}
OMClassConstantType OMClassConstantDynamic::type()
{
    return OMClassConstantType::Dynamic;
}
OMClassConstantInvokeDynamic::OMClassConstantInvokeDynamic(uint16_t bmai, uint16_t nti)
    : bootstrapMethodAttrIndex(bmai)
    , nameAndTypeIndex(nti)
{
}
OMClassConstantType OMClassConstantInvokeDynamic::type()
{
    return OMClassConstantType::InvokeDynamic;
}
OMClassConstantModule::OMClassConstantModule(uint16_t ni)
    : nameIndex(ni)
{
}
OMClassConstantType OMClassConstantModule::type()
{
    return OMClassConstantType::Module;
}
OMClassConstantPackage::OMClassConstantPackage(uint16_t ni)
    : nameIndex(ni)
{
}
OMClassConstantType OMClassConstantPackage::type()
{
    return OMClassConstantType::Package;
}

OMClassAttrConstantValue::OMClassAttrConstantValue(uint16_t vi)
    : valueIndex(vi)
{
}
OMClassAttrType OMClassAttrConstantValue::type() { return OMClassAttrType::ConstantValue; }

OMClassAttrCode::OMClassAttrCode(uint16_t ms, uint16_t ml, uint32_t cl, uint8_t* c, uint16_t etl, std::vector<OMClassAttrCodeExcTable> et, uint16_t ac, std::vector<OMClassAttr*> a)
    : maxStack(ms)
    , maxLocals(ml)
    , codeLength(cl)
    , code(c)
    , excTableLength(etl)
    , excTable(et)
    , attributesCount(ac)
    , attributes(a)
{
}
OMClassAttrType OMClassAttrCode::type() { return OMClassAttrType::Code; }

OMClassAttrStackMapTable::OMClassAttrStackMapTable(uint16_t noe, OMClassAttrVerifyStackMapFrame* e)
    : numberOfEntries(noe)
    , entries(e)
{
}
OMClassAttrType OMClassAttrStackMapTable::type() { return OMClassAttrType::StackMapTable; }

OMClassAttrExceptions::OMClassAttrExceptions(uint16_t noe, std::vector<uint16_t> eit)
    : numberOfExceptions(noe)
    , exceptionIndexTable(eit)
{
}
OMClassAttrType OMClassAttrExceptions::type() { return OMClassAttrType::Exceptions; }

OMClassAttrInnerClass::OMClassAttrInnerClass(uint16_t numberOfClasses, OMClassAttrInnerClassInfo* classes)
    : numberOfClasses(numberOfClasses)
    , classes(classes)
{
}
OMClassAttrType OMClassAttrInnerClass::type() { return OMClassAttrType::InnerClasses; }

OMClassAttrEnclosingMethod::OMClassAttrEnclosingMethod(uint16_t ci, uint16_t mi)
    : classIndex(ci)
    , methodIndex(mi)
{
}
OMClassAttrType OMClassAttrEnclosingMethod::type() { return OMClassAttrType::EnclosingMethod; }

OMClassAttrSynthetic::OMClassAttrSynthetic() { }
OMClassAttrType OMClassAttrSynthetic::type() { return OMClassAttrType::Synthetic; }

OMClassAttrSignature::OMClassAttrSignature(uint16_t si)
    : signatureIndex(si)
{
}
OMClassAttrType OMClassAttrSignature::type() { return OMClassAttrType::Signature; }

OMClassAttrSourceFile::OMClassAttrSourceFile(uint16_t si)
    : sourcefileIndex(si)
{
}
OMClassAttrType OMClassAttrSourceFile::type() { return OMClassAttrType::SourceFile; }

OMClassAttrSourceDebugExtension::OMClassAttrSourceDebugExtension(uint8_t* de)
    : debugExt(de)
{
}
OMClassAttrType OMClassAttrSourceDebugExtension::type() { return OMClassAttrType::SourceDebugExtension; }

OMClassAttrLineNumberTable::OMClassAttrLineNumberTable(uint16_t lntl, std::map<uint16_t, uint16_t> lnt)
    : lineNumberTableLength(lntl)
    , lineNumberTable(lnt)
{
}
OMClassAttrType OMClassAttrLineNumberTable::type() { return OMClassAttrType::LineNumberTable; }

OMClassAttrLocalVarTable::OMClassAttrLocalVarTable(uint16_t lvtl, OMClassAttrLocalVar* lvt)
    : localVarTableLength(lvtl)
    , localVarTable(lvt)
{
}
OMClassAttrType OMClassAttrLocalVarTable::type() { return OMClassAttrType::LocalVariableTable; }

OMClassAttrLocalVarTypeTable::OMClassAttrLocalVarTypeTable(uint16_t lvtl, OMClassAttrLocalVar* lvt)
    : localVarTableLength(lvtl)
    , localVarTable(lvt)
{
}
OMClassAttrType OMClassAttrLocalVarTypeTable::type() { return OMClassAttrType::LocalVariableTypeTable; }

OMClassAttrDeprecated::OMClassAttrDeprecated() { }
OMClassAttrType OMClassAttrDeprecated::type() { return OMClassAttrType::Deprecated; }

OMClassFileParser::OMClassFileParser(std::istream& str)
{
    this->source = &str;
    this->logger = new log::OMLogger("OMClassFileParser", this);
}

OMClassFileParser::~OMClassFileParser()
{
    this->logger->info("Destroying class file parser");
    delete this->logger;
}

OMClassFile* OMClassFileParser::parse()
{
    auto file = new OMClassFile;
    this->source->readbe32(file->magicNumber);
    this->source->readbe16(file->minor);
    this->source->readbe16(file->major);
    this->source->readbe16(file->constantPoolCount);
    file->constants = std::vector<OMClassConstant*>();

    uint16_t idx = 0;
    while (idx < file->constantPoolCount - 1) {
        file->constants.push_back(this->parseConstant(&idx));
    }

    this->source->readbe16(file->accessFlags);
    this->source->readbe16(file->thisClass);
    this->source->readbe16(file->superClass);
    this->source->readbe16(file->interfacesCount);
    file->interfaces = std::vector<uint16_t>();
    for (uint16_t d = 0; d < file->interfacesCount; d++) {
        uint16_t a;
        this->source->readbe16(a);
        file->interfaces.push_back(a);
    }
    this->source->readbe16(file->fieldsCount);
    file->fields = std::vector<OMClassFieldInfo*>();

    std::map<uint16_t, OMClassConstant*> m = buildConstantMapping(file->constants);
    for (uint16_t d = 0; d < file->fieldsCount; d++) {
        file->fields.push_back(parseField(m));
    }

    this->source->readbe16(file->methodsCount);
    file->methods = std::vector<OMClassMethodInfo*>();
    for (uint16_t d = 0; d < file->methodsCount; d++) {
        file->methods.push_back(parseMethod(m));
    }

    this->source->readbe16(file->attrCount);
    file->attrs = std::vector<OMClassAttr*>();
    for (uint16_t d = 0; d < file->attrCount; d++) {
        file->attrs.push_back(parseAttr(m));
    }

    return file;
}

OMClassConstant* OMClassFileParser::parseConstant(uint16_t* idx)
{
    (*idx)++;

    OMClassConstantType type;
    this->source->read((char*)&type, 1);

    uint16_t temp1, temp2, temp3, temp4;
    uint32_t temp5, temp6;

    OMClassConstant* result = nullptr;
    switch (type) {
    case OMClassConstantType::Class: {
        this->source->readbe16(temp1);
        result = new OMClassConstantClass(temp1);
        break;
    }
    case OMClassConstantType::FieldRef: {
        this->source->readbe16(temp1);
        this->source->readbe16(temp2);
        result = new OMClassConstantFieldRef(temp1, temp2);
        break;
    }
    case OMClassConstantType::MethodRef: {
        this->source->readbe16(temp1);
        this->source->readbe16(temp2);
        result = new OMClassConstantMethodRef(temp1, temp2);
        break;
    }
    case OMClassConstantType::InterfaceMethodRef: {
        this->source->readbe16(temp1);
        this->source->readbe16(temp2);
        result = new OMClassConstantInterfaceMethodRef(temp1, temp2);
        break;
    }
    case OMClassConstantType::NameAndType: {
        this->source->readbe16(temp1);
        this->source->readbe16(temp2);
        result = new OMClassConstantNameAndType(temp1, temp2);
        break;
    }
    case OMClassConstantType::Utf8: {
        this->source->readbe16(temp1);
        auto temp = new uint8_t[temp1];
        this->source->read((char*)temp, temp1);
        auto comp = std::string(toStdUtf8(temp, temp1));
        delete[] temp;
        result = new OMClassConstantUtf8(comp);
        break;
    }
    case OMClassConstantType::String: {
        this->source->readbe16(temp1);
        result = new OMClassConstantString(temp1);
        break;
    }
    case OMClassConstantType::Integer: {
        this->source->readbe32(temp5);
        union {
            uint32_t uidata;
            int idata;
        } d;
        d.uidata = temp5;
        result = new OMClassConstantInteger(d.idata);
        break;
    }
    case OMClassConstantType::Float: {
        this->source->readbe32(temp5);
        union {
            uint32_t idata;
            float fdata;
        } d;
        d.idata = temp5;
        result = new OMClassConstantFloat(d.fdata);
        break;
    }
    case OMClassConstantType::Long: {
        this->source->readbe32(temp5);
        this->source->readbe32(temp6);
        result = new OMClassConstantLong(((int64_t)temp5 << 32) + temp6);
        (*idx)++;
        break;
    }
    case OMClassConstantType::Double: {
        this->source->readbe32(temp5);
        this->source->readbe32(temp6);
        union {
            int64_t idata;
            double ddata;
        } d;
        d.idata = ((int64_t)temp5 << 32) + temp6;
        result = new OMClassConstantDouble(d.ddata);
        (*idx)++;
        break;
    }
    case OMClassConstantType::MethodHandle: {
        uint8_t temp;
        this->source->read((char*)&temp, 1);
        this->source->readbe16(temp1);
        result = new OMClassConstantMethodHandle(temp, temp1);
        break;
    }
    case OMClassConstantType::MethodType: {
        this->source->readbe16(temp1);
        result = new OMClassConstantMethodType(temp1);
        break;
    }
    case OMClassConstantType::Dynamic: {
        this->source->readbe16(temp1);
        this->source->readbe16(temp2);
        result = new OMClassConstantDynamic(temp1, temp2);
        break;
    }
    case OMClassConstantType::InvokeDynamic: {
        this->source->readbe16(temp1);
        this->source->readbe16(temp2);
        result = new OMClassConstantInvokeDynamic(temp1, temp2);
        break;
    }
    case OMClassConstantType::Module: {
        this->source->readbe16(temp1);
        result = new OMClassConstantModule(temp1);
        break;
    }
    case OMClassConstantType::Package: {
        this->source->readbe16(temp1);
        result = new OMClassConstantPackage(temp1);
        break;
    }
    default: {
        throw std::invalid_argument("Unknown constant type id!");
        break;
    }
    }

    return result;
}

std::map<uint16_t, OMClassConstant*> OMClassFileParser::buildConstantMapping(std::vector<OMClassConstant*> c)
{
    std::map<uint16_t, OMClassConstant*> target;
    uint16_t id = 1;
    for (auto d : c) {
        target[id] = d;
        if (d->type() == OMClassConstantType::Long || d->type() == OMClassConstantType::Double) {
            id++;
        }
        id++;
    }

    return target;
}

OMClassFieldInfo* OMClassFileParser::parseField(std::map<uint16_t, OMClassConstant*> m)
{
    auto field = new OMClassFieldInfo;
    this->source->readbe16(field->accessFlags);
    this->source->readbe16(field->nameIndex);
    this->source->readbe16(field->descIndex);
    this->source->readbe16(field->attrCount);
    field->attrs = std::vector<OMClassAttr*>();
    for (uint16_t c = 0; c < field->attrCount; c++) {
        field->attrs.push_back(parseAttr(m));
    }

    return field;
}

OMClassMethodInfo* OMClassFileParser::parseMethod(std::map<uint16_t, OMClassConstant*> m)
{
    auto method = new OMClassMethodInfo;
    this->source->readbe16(method->accessFlags);
    this->source->readbe16(method->nameIndex);
    this->source->readbe16(method->descIndex);
    this->source->readbe16(method->attrCount);
    method->attrs = std::vector<OMClassAttr*>();
    for (uint16_t c = 0; c < method->attrCount; c++) {
        method->attrs.push_back(parseAttr(m));
    }

    return method;
}

OMClassAttr* OMClassFileParser::parseAttr(std::map<uint16_t, OMClassConstant*> m)
{
    uint16_t ni;
    uint32_t length;
    this->source->readbe16(ni);
    this->source->readbe32(length);

    if (m[ni]->type() != OMClassConstantType::Utf8) {
        throw std::invalid_argument("Invalid attr name index!");
    }

    OMClassAttr* attr;

    switch (hash_compile_time(m[ni]->to<OMClassConstantUtf8>()->data.c_str())) {
    case "ConstantValue"_hash: {
        uint16_t cvi;
        this->source->readbe16(cvi);
        attr = new OMClassAttrConstantValue(cvi);
        break;
    }
    case "Code"_hash: {
        uint16_t ms, ml, etl, ac;
        uint32_t cl;
        this->source->readbe16(ms);
        this->source->readbe16(ml);
        this->source->readbe32(cl);
        auto code = new uint8_t[cl];
        this->source->read((char*)code, cl);
        this->source->readbe16(etl);
        std::vector<OMClassAttrCodeExcTable> et;
        for (uint16_t i = 0; i < etl; i++) {
            uint16_t sp, ep, hp, ct;
            this->source->readbe32(sp);
            this->source->readbe32(ep);
            this->source->readbe32(hp);
            this->source->readbe32(ct);
            et.push_back({ sp, ep, hp, ct });
        }
        this->source->readbe16(ac);
        std::vector<OMClassAttr*> a;
        for (uint16_t i = 0; i < ac; i++) {
            a.push_back(parseAttr(m));
        }
        attr = new OMClassAttrCode(ms, ml, cl, code, etl, et, ac, a);
        break;
    }
    case "StackMapTable"_hash: {
        uint16_t noe;
        this->source->readbe16(noe);
        auto typep = [&]() -> OMClassAttrVerifyTypeInfo {
            OMClassAttrVerifyTypeInfo s;
            this->source->read((char*)&s.tag, 1);
            if (s.tag == OMClassAttrVerifyType::Object || s.tag == OMClassAttrVerifyType::Uninitialized) {
                this->source->readbe16(s.arg);
            }
            return s;
        };
        std::vector<OMClassAttrVerifyStackMapFrame> datas;
        for (uint16_t i = 0; i < noe; i++) {
            OMClassAttrVerifyStackMapFrame fr;
            this->source->read((char*)&fr.tag, 1);

            if (fr.tag >= 64 && fr.tag < 128) {
                fr.sameLocals1StackItemFrame.stack = typep();
            } else if (fr.tag < 247) {
                throw std::invalid_argument("Invalid stack map frame type!");
            } else if (fr.tag == 247) {
                this->source->readbe16(fr.sameLocals1StackItemFrameExt.offset);
                fr.sameLocals1StackItemFrameExt.stack = typep();
            } else if (fr.tag < 251) {
                this->source->readbe16(fr.chopFrame.offset);
            } else if (fr.tag == 251) {
                this->source->readbe16(fr.sameFrameExt.offset);
            } else if (fr.tag < 255) {
                this->source->readbe16(fr.appendFrame.offset);
                fr.appendFrame.locals = std::vector<OMClassAttrVerifyTypeInfo>();
                for (uint8_t i = 0; i < fr.tag - 251; i++) {
                    fr.appendFrame.locals.push_back(typep());
                }
            } else {
                this->source->readbe16(fr.fullFrame.offset);
                this->source->readbe16(fr.fullFrame.numberOfLocals);
                fr.fullFrame.locals = std::vector<OMClassAttrVerifyTypeInfo>();
                for (uint16_t i = 0; i < fr.fullFrame.numberOfLocals; i++) {
                    fr.fullFrame.locals.push_back(typep());
                }
                this->source->readbe16(fr.fullFrame.numberOfStackItems);
                fr.fullFrame.stackItems = std::vector<OMClassAttrVerifyTypeInfo>();
                for (uint16_t i = 0; i < fr.fullFrame.numberOfStackItems; i++) {
                    fr.fullFrame.stackItems.push_back(typep());
                }
            }
        }
        attr = new OMClassAttrStackMapTable(noe, datas.data());
        break;
    }
    case "Exceptions"_hash: {
        uint16_t cnt;
        this->source->readbe16(cnt);
        std::vector<uint16_t> excindex;
        for (uint16_t i = 0; i < cnt; i++) {
            uint16_t d;
            this->source->readbe16(d);
            excindex.push_back(d);
        }
        attr = new OMClassAttrExceptions(cnt, excindex);
        break;
    }
    case "InnerClasses"_hash: {
        uint16_t numberOfClasses;
        this->source->readbe16(numberOfClasses);
        std::vector<OMClassAttrInnerClassInfo> d;
        for (uint16_t i = 0; i < numberOfClasses; i++) {
            OMClassAttrInnerClassInfo di;
            this->source->readbe16(di.innerClassInfoIndex);
            this->source->readbe16(di.outerClassInfoIndex);
            this->source->readbe16(di.innerNameIndex);
            this->source->readbe16(di.innerClassAccessFlags);
            d.push_back(di);
        }
        attr = new OMClassAttrInnerClass(numberOfClasses, d.data());
        break;
    }
    case "EnclosingMethod"_hash: {
        uint16_t ci, mi;
        this->source->readbe16(ci);
        this->source->readbe16(mi);
        attr = new OMClassAttrEnclosingMethod(ci, mi);
        break;
    }
    case "Synthetic"_hash: {
        attr = new OMClassAttrSynthetic;
        break;
    }
    case "Signature"_hash: {
        uint16_t si;
        this->source->readbe16(si);
        attr = new OMClassAttrSignature(si);
        break;
    }
    case "SourceFile"_hash: {
        uint16_t si;
        this->source->readbe16(si);
        attr = new OMClassAttrSourceFile(si);
        break;
    }
    case "SourceDebugExtension"_hash: {
        auto data = new uint8_t[length];
        this->source->read((char*)data, length);
        attr = new OMClassAttrSourceDebugExtension(data);
        break;
    }
    case "LineNumberTable"_hash: {
        uint16_t lntl, a, b;
        this->source->readbe16(lntl);
        std::map<uint16_t, uint16_t> lnt;
        for (uint16_t i = 0; i < lntl; i++) {
            this->source->readbe16(a);
            this->source->readbe16(b);
            lnt[a] = b;
        }
        attr = new OMClassAttrLineNumberTable(lntl, lnt);
        break;
    }
    case "LocalVariableTable"_hash: {
        uint16_t l;
        this->source->readbe16(l);
        std::vector<OMClassAttrLocalVar> d;
        for (uint16_t i = 0; i < l; i++) {
            OMClassAttrLocalVar data;
            this->source->readbe16(data.startPc);
            this->source->readbe16(data.length);
            this->source->readbe16(data.nameIndex);
            this->source->readbe16(data.descIndex);
            this->source->readbe16(data.index);
            d.push_back(data);
        }
        attr = new OMClassAttrLocalVarTable(l, d.data());
        break;
    }
    case "LocalVariableTypeTable"_hash: {
        uint16_t l;
        this->source->readbe16(l);
        std::vector<OMClassAttrLocalVar> d;
        for (uint16_t i = 0; i < l; i++) {
            OMClassAttrLocalVar data;
            this->source->readbe16(data.startPc);
            this->source->readbe16(data.length);
            this->source->readbe16(data.nameIndex);
            this->source->readbe16(data.descIndex);
            this->source->readbe16(data.index);
            d.push_back(data);
        }
        attr = new OMClassAttrLocalVarTypeTable(l, d.data());
        break;
    }
    case "Deprecated"_hash: {
        attr = new OMClassAttrDeprecated;
        break;
    }
    default:
        this->source->seekg((uint64_t)this->source->tellg() + length);
        this->logger->info("Unimplemented attr: {}", m[ni]->to<OMClassConstantUtf8>()->data);
        break;
    }

    return attr;
}

char* OMClassFileParser::toStdUtf8(uint8_t* data, int length)
{
    int p = 0;
    auto target = new std::vector<char>();
    while (p < length) {
        if (data[p] >> 7 == 0) {
            target->push_back(data[p]);
            p += 1;
            continue;
        }

        if (data[p] >> 5 == 0b110 && data[p + 1] >> 6 == 0b10) {
            auto d = ((data[p] & 0x1f) << 6) + (data[p + 1] & 0x3f);
            if (d != 0)
                target->push_back(d);
            p += 2;
            continue;
        }

        if (data[p] >> 4 == 0b1110 && data[p + 1] >> 6 == 0b10 && data[p + 2] >> 5 == 0b10) {
            target->push_back(((data[p] & 0xf) << 12) + ((data[p + 1] & 0x3f) << 6) + (data[p + 2] & 0x3f));
            p += 3;
            continue;
        }

        if (data[p] == 0b11101101 && data[p + 1] >> 4 == 0b1010 && data[p + 2] >> 6 == 0b10 && data[p + 3] == 0b11101101 && data[p + 4] >> 4 == 0b1011 && data[p + 5] >> 6 == 0b10) {
            target->push_back(0x10000 + ((data[p + 1] & 0x0f) << 16) + ((data[p + 2] & 0x3f) << 10) + ((data[p + 4] & 0x0f) << 6) + (data[p + 5] & 0x3f));
            p += 6;
            continue;
        }
    }
    target->push_back('\0');

    return target->data();
}
} // namespace openminecraft::vm::classfile
