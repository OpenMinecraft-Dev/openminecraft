#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <cstdint>
#include <fmt/format.h>
#include <memory>
#include <openminecraft/binary/om_bin_endians.hpp>
#include <openminecraft/vm/om_class_file.hpp>
#include <stdexcept>

using namespace openminecraft::binary;
using namespace openminecraft::binary::hash;

namespace openminecraft::vm::classfile {
OMClassFileParser::OMClassFileParser(std::istream& str)
{
    this->source = &str;
    this->logger = std::make_shared<log::OMLogger>("OMClassFileParser", this);
}

OMClassFileParser::~OMClassFileParser()
{
    this->logger->info("Destroying class file parser");
}

std::shared_ptr<OMClassFile> OMClassFileParser::parse()
{
    auto file = std::make_shared<OMClassFile>();
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
        attr = new OMClassAttrInnerClass(numberOfClasses, d);
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
    case "RuntimeVisibleAnnotations"_hash: {
        uint16_t na;
        this->source->readbe16(na);
        std::vector<OMClassAnnotation*> d;
        for (uint16_t i = 0; i < na; i++) {
            d.push_back(parseAnnotation());
        }
        attr = new OMClassAttrRuntimeVisibleAnnotations(na, d);
        break;
    }
    case "RuntimeInvisibleAnnotations"_hash: {
        uint16_t na;
        this->source->readbe16(na);
        std::vector<OMClassAnnotation*> d;
        for (uint16_t i = 0; i < na; i++) {
            d.push_back(parseAnnotation());
        }
        attr = new OMClassAttrRuntimeInvisibleAnnotations(na, d);
        break;
    }
    case "RuntimeVisibleParameterAnnotations"_hash: {
        uint8_t n;
        this->source->read((char*)&n, 1);
        std::vector<OMClassParamAnnotations> d;
        for (uint8_t i = 0; i < n; i++) {
            std::vector<OMClassAnnotation*> d0;
            uint16_t ca;
            this->source->readbe16(ca);
            for (uint16_t j = 0; j < ca; j++) {
                d0.push_back(parseAnnotation());
            }
            d.push_back({ ca, d0 });
        }
        attr = new OMClassRuntimeVisibleParameterAnnotations(n, d.data());
        break;
    }
    case "RuntimeInvisibleParameterAnnotations"_hash: {
        uint8_t n;
        this->source->read((char*)&n, 1);
        std::vector<OMClassParamAnnotations> d;
        for (uint8_t i = 0; i < n; i++) {
            std::vector<OMClassAnnotation*> d0;
            uint16_t ca;
            this->source->readbe16(ca);
            for (uint16_t j = 0; j < ca; j++) {
                d0.push_back(parseAnnotation());
            }
            d.push_back({ ca, d0 });
        }
        attr = new OMClassRuntimeInvisibleParameterAnnotations(n, d.data());
        break;
    }
    // RuntimeVisibleTypeAnnotations
    // RuntimeInvisibleTypeAnnotations
    case "AnnotationDefault"_hash: {
        attr = new OMClassAttrAnnotationDefault(parseAnnotationValue());
        break;
    }
    case "BootstrapMethods"_hash: {
        uint16_t n;
        std::vector<OMClassBootMethods> data;
        this->source->readbe16(n);
        for (uint16_t i = 0; i < n; i++) {
            uint16_t ref, c;
            std::vector<uint16_t> d;
            this->source->readbe16(ref);
            this->source->readbe16(c);
            for (uint16_t j = 0; j < c; j++) {
                uint16_t di;
                this->source->readbe16(di);
                d.push_back(di);
            }
            data.push_back({ ref, c, d });
        }
        attr = new OMClassAttrBootMethods(n, data.data());
        break;
    }
    case "MethodParameters"_hash: {
        uint8_t pc;
        uint16_t a, b;
        std::vector<OMClassParam> d;
        this->source->read((char*)&pc, 1);
        for (uint8_t i = 0; i < pc; i++) {
            this->source->readbe16(a);
            this->source->readbe16(b);
            d.push_back({ a, b });
        }
        attr = new OMClassAttrMethodParameters(pc, d.data());
        break;
    }
    // Module
    case "ModulePackages"_hash: {
        uint16_t pc;
        std::vector<uint16_t> data;
        this->source->readbe16(pc);
        for (uint16_t i = 0; i < pc; i++) {
            uint16_t d;
            this->source->readbe16(d);
            data.push_back(d);
        }
        attr = new OMClassAttrModulePackages(pc, data);
        break;
    }
    case "ModuleMainClass"_hash: {
        uint16_t mci;
        this->source->readbe16(mci);
        attr = new OMClassAttrModuleMainClass(mci);
        break;
    }
    case "NestHost"_hash: {
        uint16_t d;
        this->source->readbe16(d);
        attr = new OMClassAttrNestHost(d);
        break;
    }
    case "NestMembers"_hash: {
        uint16_t noc;
        std::vector<uint16_t> data;
        this->source->readbe16(noc);
        for (uint16_t i = 0; i < noc; i++) {
            uint16_t d;
            this->source->readbe16(d);
            data.push_back(d);
        }
        attr = new OMClassAttrNestMembers(noc, data);
        break;
    }
    case "Record"_hash: {
        uint16_t c;
        this->source->readbe16(c);
        auto da = new OMClassRecordCompInfo[c];
        for (uint16_t i = 0; i < c; i++) {
            uint16_t ni, di, ac;
            std::vector<OMClassAttr*> d;
            this->source->readbe16(ni);
            this->source->readbe16(di);
            this->source->readbe16(ac);
            for (uint16_t j = 0; j < ac; j++) {
                d.push_back(parseAttr(m));
            }
            da[i] = { ni, di, ac, d };
        }
        attr = new OMClassAttrRecord(c, da);
        break;
    }
    case "PermittedSubclasses"_hash: {
        uint16_t noc;
        std::vector<uint16_t> data;
        this->source->readbe16(noc);
        for (uint16_t i = 0; i < noc; i++) {
            uint16_t d;
            this->source->readbe16(d);
            data.push_back(d);
        }
        attr = new OMClassAttrPermittedSubclasses(noc, data);
        break;
    }
    default:
        this->source->seekg((uint64_t)this->source->tellg() + length);
        this->logger->info("Unimplemented attr: {}", m[ni]->to<OMClassConstantUtf8>()->data);
        break;
    }

    return attr;
}

OMClassAnnotation* OMClassFileParser::parseAnnotation()
{
    this->source->seekg((uint64_t)this->source->tellg() + 2);
    auto anno = new OMClassAnnotation;
    this->source->readbe16(anno->type);
    this->source->readbe16(anno->numPairs);
    anno->pairs = std::map<uint16_t, OMClassAnnotationElemValue*>();

    for (uint16_t idx = 0; idx < anno->numPairs; idx++) {
        uint16_t i;
        this->source->readbe16(i);
        anno->pairs[i] = parseAnnotationValue();
    }

    return anno;
}

OMClassAnnotationElemValue* OMClassFileParser::parseAnnotationValue()
{
    auto v = new OMClassAnnotationElemValue;
    this->source->read((char*)&v->tag, 1);
    switch (v->tag) {
    case 'B':
    case 'C':
    case 'D':
    case 'F':
    case 'I':
    case 'J':
    case 'S':
    case 'Z':
    case 's':
        this->source->readbe16(v->value.constValueIndex);
        break;
    case 'e':
        this->source->readbe16(v->value.enumConstValue.typeNameIndex);
        this->source->readbe16(v->value.enumConstValue.constNameIndex);
        break;
    case 'c':
        this->source->readbe16(v->value.classInfoIndex);
        break;
    case '@':
        v->value.annotationValue = parseAnnotation();
        break;
    case '[': {
        std::vector<OMClassAnnotationElemValue> d;
        this->source->readbe16(v->value.arrayValue.numValues);
        for (uint16_t i = 0; i < v->value.arrayValue.numValues; i++) {
            d.push_back(*parseAnnotationValue());
        }
        v->value.arrayValue.values = d.data();
        break;
    }
    default:
        return nullptr;
    }
    return v;
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
