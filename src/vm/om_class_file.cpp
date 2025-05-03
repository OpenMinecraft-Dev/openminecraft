#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/io/om_io_parser.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include <cstdint>
#include <fmt/format.h>
#include <memory>
#include <openminecraft/binary/om_bin_endians.hpp>
#include <openminecraft/vm/om_class_file.hpp>
#include <stdexcept>

using namespace openminecraft::binary;
using namespace openminecraft::binary::hash;
using namespace openminecraft::util;

namespace openminecraft::vm::classfile
{
OMClassFileParser::OMClassFileParser(std::shared_ptr<std::istream> str) : io::OMParser(str)
{
    this->logger = std::make_shared<log::OMLogger>("OMClassFileParser", this);
}

OMClassFileParser::~OMClassFileParser()
{
    this->logger->info("Destroying class file parser");
    io::OMParser::~OMParser();
}

util::OMResult<std::shared_ptr<OMClassFile>, std::exception> OMClassFileParser::parse()
{
    auto file = std::make_shared<OMClassFile>();
    this->source->readbe32(file->magicNumber);
    if (file->magicNumber != 0xcafebabe)
    {
        return OMResult<std::shared_ptr<OMClassFile>, std::exception>::err(
            std::invalid_argument("invalid class file magic number!"));
    }
    this->source->readbe16(file->minor);
    this->source->readbe16(file->major);
    this->source->readbe16(file->constantPoolCount);
    file->constants = std::vector<std::shared_ptr<OMClassConstant>>();

    uint16_t idx = 0;
    while (idx < file->constantPoolCount - 1)
    {
        auto c = this->parseConstant(&idx);
        switch (c.type)
        {
        case Ok: {
            file->constants.push_back(c.unwrap());
            break;
        }
        case Err: {
            return OMResult<std::shared_ptr<OMClassFile>, std::exception>::err(c.unwrap_err());
        }
        }
    }

    this->source->readbe16(file->accessFlags);
    this->source->readbe16(file->thisClass);
    this->source->readbe16(file->superClass);
    this->source->readbe16(file->interfacesCount);
    file->interfaces = std::vector<uint16_t>();
    for (uint16_t d = 0; d < file->interfacesCount; d++)
    {
        uint16_t a;
        this->source->readbe16(a);
        file->interfaces.push_back(a);
    }
    this->source->readbe16(file->fieldsCount);
    file->fields = std::vector<std::shared_ptr<OMClassFieldInfo>>();

    OMClassFileParser::ConstantMapping m = buildConstantMapping(file->constants);
    for (uint16_t d = 0; d < file->fieldsCount; d++)
    {
        file->fields.push_back(parseField(m));
    }

    this->source->readbe16(file->methodsCount);
    file->methods = std::vector<std::shared_ptr<OMClassMethodInfo>>();
    for (uint16_t d = 0; d < file->methodsCount; d++)
    {
        file->methods.push_back(parseMethod(m));
    }

    this->source->readbe16(file->attrCount);
    file->attrs = std::vector<std::shared_ptr<OMClassAttr>>();
    for (uint16_t d = 0; d < file->attrCount; d++)
    {
        file->attrs.push_back(parseAttr(m));
    }

    return OMResult<std::shared_ptr<OMClassFile>, std::exception>::ok(file);
}

OMResult<std::shared_ptr<OMClassConstant>, std::exception> OMClassFileParser::parseConstant(uint16_t *idx)
{
    (*idx)++;

    OMClassConstantType type;
    this->source->read((char *)&type, 1);

    uint16_t temp1, temp2, temp3, temp4;
    uint32_t temp5, temp6;

    std::shared_ptr<OMClassConstant> result;
    switch (type)
    {
    case OMClassConstantType::Class: {
        this->source->readbe16(temp1);
        result = std::make_shared<OMClassConstantClass>(temp1);
        break;
    }
    case OMClassConstantType::FieldRef: {
        this->source->readbe16(temp1);
        this->source->readbe16(temp2);
        result = std::make_shared<OMClassConstantFieldRef>(temp1, temp2);
        break;
    }
    case OMClassConstantType::MethodRef: {
        this->source->readbe16(temp1);
        this->source->readbe16(temp2);
        result = std::make_shared<OMClassConstantMethodRef>(temp1, temp2);
        break;
    }
    case OMClassConstantType::InterfaceMethodRef: {
        this->source->readbe16(temp1);
        this->source->readbe16(temp2);
        result = std::make_shared<OMClassConstantInterfaceMethodRef>(temp1, temp2);
        break;
    }
    case OMClassConstantType::NameAndType: {
        this->source->readbe16(temp1);
        this->source->readbe16(temp2);
        result = std::make_shared<OMClassConstantNameAndType>(temp1, temp2);
        break;
    }
    case OMClassConstantType::Utf8: {
        this->source->readbe16(temp1);
        std::vector<uint8_t> temp(temp1);
        this->source->read((char *)temp.data(), temp1);
        auto comp = toStdUtf8(temp, temp1);
        result = std::make_shared<OMClassConstantUtf8>(comp);
        break;
    }
    case OMClassConstantType::String: {
        this->source->readbe16(temp1);
        result = std::make_shared<OMClassConstantString>(temp1);
        break;
    }
    case OMClassConstantType::Integer: {
        this->source->readbe32(temp5);
        result = std::make_shared<OMClassConstantInteger>(*((int *)&temp5));
        break;
    }
    case OMClassConstantType::Float: {
        this->source->readbe32(temp5);
        result = std::make_shared<OMClassConstantFloat>(*((float *)&temp5));
        break;
    }
    case OMClassConstantType::Long: {
        this->source->readbe32(temp5);
        this->source->readbe32(temp6);
        result = std::make_shared<OMClassConstantLong>(((int64_t)temp5 << 32) + temp6);
        (*idx)++;
        break;
    }
    case OMClassConstantType::Double: {
        this->source->readbe32(temp5);
        this->source->readbe32(temp6);
        auto temp = ((int64_t)temp5 << 32) + temp6;
        result = std::make_shared<OMClassConstantDouble>(*((double *)&temp));
        (*idx)++;
        break;
    }
    case OMClassConstantType::MethodHandle: {
        uint8_t temp;
        this->source->read((char *)&temp, 1);
        this->source->readbe16(temp1);
        result = std::make_shared<OMClassConstantMethodHandle>(temp, temp1);
        break;
    }
    case OMClassConstantType::MethodType: {
        this->source->readbe16(temp1);
        result = std::make_shared<OMClassConstantMethodType>(temp1);
        break;
    }
    case OMClassConstantType::Dynamic: {
        this->source->readbe16(temp1);
        this->source->readbe16(temp2);
        result = std::make_shared<OMClassConstantDynamic>(temp1, temp2);
        break;
    }
    case OMClassConstantType::InvokeDynamic: {
        this->source->readbe16(temp1);
        this->source->readbe16(temp2);
        result = std::make_shared<OMClassConstantInvokeDynamic>(temp1, temp2);
        break;
    }
    case OMClassConstantType::Module: {
        this->source->readbe16(temp1);
        result = std::make_shared<OMClassConstantModule>(temp1);
        break;
    }
    case OMClassConstantType::Package: {
        this->source->readbe16(temp1);
        result = std::make_shared<OMClassConstantPackage>(temp1);
        break;
    }
    default: {
        return OMResult<std::shared_ptr<OMClassConstant>, std::exception>::err(
            std::invalid_argument("Unknown constant type id!"));
        break;
    }
    }

    return OMResult<std::shared_ptr<OMClassConstant>, std::exception>::ok(result);
}

OMClassFileParser::ConstantMapping OMClassFileParser::buildConstantMapping(
    std::vector<std::shared_ptr<OMClassConstant>> c)
{
    OMClassFileParser::ConstantMapping target;
    uint16_t id = 1;
    for (auto d : c)
    {
        target[id] = d;
        if (d->type() == OMClassConstantType::Long || d->type() == OMClassConstantType::Double)
        {
            id++;
        }
        id++;
    }

    return target;
}

std::shared_ptr<OMClassFieldInfo> OMClassFileParser::parseField(OMClassFileParser::ConstantMapping m)
{
    auto field = std::make_shared<OMClassFieldInfo>();
    this->source->readbe16(field->accessFlags);
    this->source->readbe16(field->nameIndex);
    this->source->readbe16(field->descIndex);
    this->source->readbe16(field->attrCount);
    field->attrs = std::vector<std::shared_ptr<OMClassAttr>>();
    for (uint16_t c = 0; c < field->attrCount; c++)
    {
        field->attrs.push_back(parseAttr(m));
    }

    return field;
}

std::shared_ptr<OMClassMethodInfo> OMClassFileParser::parseMethod(OMClassFileParser::ConstantMapping m)
{
    auto method = std::make_shared<OMClassMethodInfo>();
    this->source->readbe16(method->accessFlags);
    this->source->readbe16(method->nameIndex);
    this->source->readbe16(method->descIndex);
    this->source->readbe16(method->attrCount);
    method->attrs = std::vector<std::shared_ptr<OMClassAttr>>();
    for (uint16_t c = 0; c < method->attrCount; c++)
    {
        method->attrs.push_back(parseAttr(m));
    }

    return method;
}

std::shared_ptr<OMClassAttr> OMClassFileParser::parseAttr(OMClassFileParser::ConstantMapping m)
{
    uint16_t ni;
    uint32_t length;
    this->source->readbe16(ni);
    this->source->readbe32(length);

    if (m[ni]->type() != OMClassConstantType::Utf8)
    {
        throw std::invalid_argument("Invalid attr name index!");
    }

    std::shared_ptr<OMClassAttr> attr;

    switch (hash_compile_time(m[ni]->to<OMClassConstantUtf8>()->data.c_str()))
    {
    case "ConstantValue"_hash: {
        uint16_t cvi;
        this->source->readbe16(cvi);
        attr = std::make_shared<OMClassAttrConstantValue>(cvi);
        break;
    }
    case "Code"_hash: {
        uint16_t ms, ml, etl, ac;
        uint32_t cl;
        this->source->readbe16(ms);
        this->source->readbe16(ml);
        this->source->readbe32(cl);
        std::vector<uint8_t> code(cl);
        this->source->read((char *)code.data(), cl);
        this->source->readbe16(etl);
        std::vector<OMClassAttrCodeExcTable> et;
        for (uint16_t i = 0; i < etl; i++)
        {
            uint16_t sp, ep, hp, ct;
            this->source->readbe32(sp);
            this->source->readbe32(ep);
            this->source->readbe32(hp);
            this->source->readbe32(ct);
            et.push_back({sp, ep, hp, ct});
        }
        this->source->readbe16(ac);
        std::vector<std::shared_ptr<OMClassAttr>> a;
        for (uint16_t i = 0; i < ac; i++)
        {
            a.push_back(parseAttr(m));
        }
        attr = std::make_shared<OMClassAttrCode>(ms, ml, cl, code, etl, et, ac, a);
        break;
    }
    case "StackMapTable"_hash: {
        uint16_t noe;
        this->source->readbe16(noe);
        auto typep = [&]() -> OMClassAttrVerifyTypeInfo {
            OMClassAttrVerifyTypeInfo s;
            this->source->read((char *)&s.tag, 1);
            if (s.tag == OMClassAttrVerifyType::Object || s.tag == OMClassAttrVerifyType::Uninitialized)
            {
                this->source->readbe16(s.arg);
            }
            return s;
        };
        std::vector<std::shared_ptr<OMClassAttrVerifyStackMapFrame>> datas;
        for (uint16_t i = 0; i < noe; i++)
        {
            auto fr = std::make_shared<OMClassAttrVerifyStackMapFrame>();
            this->source->read((char *)&fr->tag, 1);

            if (fr->tag >= 64 && fr->tag < 128)
            {
                fr->sameLocals1StackItemFrame.stack = typep();
            }
            else if (fr->tag < 247)
            {
                throw std::invalid_argument("Invalid stack map frame type!");
            }
            else if (fr->tag == 247)
            {
                this->source->readbe16(fr->sameLocals1StackItemFrameExt.offset);
                fr->sameLocals1StackItemFrameExt.stack = typep();
            }
            else if (fr->tag < 251)
            {
                this->source->readbe16(fr->chopFrame.offset);
            }
            else if (fr->tag == 251)
            {
                this->source->readbe16(fr->sameFrameExt.offset);
            }
            else if (fr->tag < 255)
            {
                this->source->readbe16(fr->appendFrame.offset);
                fr->appendFrame.locals = std::vector<OMClassAttrVerifyTypeInfo>();
                for (uint8_t i = 0; i < fr->tag - 251; i++)
                {
                    fr->appendFrame.locals.push_back(typep());
                }
            }
            else
            {
                this->source->readbe16(fr->fullFrame.offset);
                this->source->readbe16(fr->fullFrame.numberOfLocals);
                fr->fullFrame.locals = std::vector<OMClassAttrVerifyTypeInfo>();
                for (uint16_t i = 0; i < fr->fullFrame.numberOfLocals; i++)
                {
                    fr->fullFrame.locals.push_back(typep());
                }
                this->source->readbe16(fr->fullFrame.numberOfStackItems);
                fr->fullFrame.stackItems = std::vector<OMClassAttrVerifyTypeInfo>();
                for (uint16_t i = 0; i < fr->fullFrame.numberOfStackItems; i++)
                {
                    fr->fullFrame.stackItems.push_back(typep());
                }
            }

            datas.push_back(fr);
        }
        attr = std::make_shared<OMClassAttrStackMapTable>(noe, datas);
        break;
    }
    case "Exceptions"_hash: {
        uint16_t cnt;
        this->source->readbe16(cnt);
        std::vector<uint16_t> excindex;
        for (uint16_t i = 0; i < cnt; i++)
        {
            uint16_t d;
            this->source->readbe16(d);
            excindex.push_back(d);
        }
        attr = std::make_shared<OMClassAttrExceptions>(cnt, excindex);
        break;
    }
    case "InnerClasses"_hash: {
        uint16_t numberOfClasses;
        this->source->readbe16(numberOfClasses);
        std::vector<OMClassAttrInnerClassInfo> d;
        for (uint16_t i = 0; i < numberOfClasses; i++)
        {
            OMClassAttrInnerClassInfo di;
            this->source->readbe16(di.innerClassInfoIndex);
            this->source->readbe16(di.outerClassInfoIndex);
            this->source->readbe16(di.innerNameIndex);
            this->source->readbe16(di.innerClassAccessFlags);
            d.push_back(di);
        }
        attr = std::make_shared<OMClassAttrInnerClass>(numberOfClasses, d);
        break;
    }
    case "EnclosingMethod"_hash: {
        uint16_t ci, mi;
        this->source->readbe16(ci);
        this->source->readbe16(mi);
        attr = std::make_shared<OMClassAttrEnclosingMethod>(ci, mi);
        break;
    }
    case "Synthetic"_hash: {
        attr = std::make_shared<OMClassAttrSynthetic>();
        break;
    }
    case "Signature"_hash: {
        uint16_t si;
        this->source->readbe16(si);
        attr = std::make_shared<OMClassAttrSignature>(si);
        break;
    }
    case "SourceFile"_hash: {
        uint16_t si;
        this->source->readbe16(si);
        attr = std::make_shared<OMClassAttrSourceFile>(si);
        break;
    }
    case "SourceDebugExtension"_hash: {
        std::vector<uint8_t> data(length);
        this->source->read((char *)data.data(), length);
        attr = std::make_shared<OMClassAttrSourceDebugExtension>(data);
        break;
    }
    case "LineNumberTable"_hash: {
        uint16_t lntl, a, b;
        this->source->readbe16(lntl);
        std::map<uint16_t, uint16_t> lnt;
        for (uint16_t i = 0; i < lntl; i++)
        {
            this->source->readbe16(a);
            this->source->readbe16(b);
            lnt[a] = b;
        }
        attr = std::make_shared<OMClassAttrLineNumberTable>(lntl, lnt);
        break;
    }
    case "LocalVariableTable"_hash: {
        uint16_t l;
        this->source->readbe16(l);
        std::vector<OMClassAttrLocalVar> d;
        for (uint16_t i = 0; i < l; i++)
        {
            OMClassAttrLocalVar data;
            this->source->readbe16(data.startPc);
            this->source->readbe16(data.length);
            this->source->readbe16(data.nameIndex);
            this->source->readbe16(data.descIndex);
            this->source->readbe16(data.index);
            d.push_back(data);
        }
        attr = std::make_shared<OMClassAttrLocalVarTable>(l, d);
        break;
    }
    case "LocalVariableTypeTable"_hash: {
        uint16_t l;
        this->source->readbe16(l);
        std::vector<OMClassAttrLocalVar> d;
        for (uint16_t i = 0; i < l; i++)
        {
            OMClassAttrLocalVar data;
            this->source->readbe16(data.startPc);
            this->source->readbe16(data.length);
            this->source->readbe16(data.nameIndex);
            this->source->readbe16(data.descIndex);
            this->source->readbe16(data.index);
            d.push_back(data);
        }
        attr = std::make_shared<OMClassAttrLocalVarTypeTable>(l, d);
        break;
    }
    case "Deprecated"_hash: {
        attr = std::make_shared<OMClassAttrDeprecated>();
        break;
    }
    case "RuntimeVisibleAnnotations"_hash: {
        uint16_t na;
        this->source->readbe16(na);
        std::vector<std::shared_ptr<OMClassAnnotation>> d;
        for (uint16_t i = 0; i < na; i++)
        {
            d.push_back(parseAnnotation());
        }
        attr = std::make_shared<OMClassAttrRuntimeVisibleAnnotations>(na, d);
        break;
    }
    case "RuntimeInvisibleAnnotations"_hash: {
        uint16_t na;
        this->source->readbe16(na);
        std::vector<std::shared_ptr<OMClassAnnotation>> d;
        for (uint16_t i = 0; i < na; i++)
        {
            d.push_back(parseAnnotation());
        }
        attr = std::make_shared<OMClassAttrRuntimeInvisibleAnnotations>(na, d);
        break;
    }
    case "RuntimeVisibleParameterAnnotations"_hash: {
        uint8_t n;
        this->source->read((char *)&n, 1);
        std::vector<OMClassParamAnnotations> d;
        for (uint8_t i = 0; i < n; i++)
        {
            std::vector<std::shared_ptr<OMClassAnnotation>> d0;
            uint16_t ca;
            this->source->readbe16(ca);
            for (uint16_t j = 0; j < ca; j++)
            {
                d0.push_back(parseAnnotation());
            }
            d.push_back({ca, d0});
        }
        attr = std::make_shared<OMClassRuntimeVisibleParameterAnnotations>(n, d);
        break;
    }
    case "RuntimeInvisibleParameterAnnotations"_hash: {
        uint8_t n;
        this->source->read((char *)&n, 1);
        std::vector<OMClassParamAnnotations> d;
        for (uint8_t i = 0; i < n; i++)
        {
            std::vector<std::shared_ptr<OMClassAnnotation>> d0;
            uint16_t ca;
            this->source->readbe16(ca);
            for (uint16_t j = 0; j < ca; j++)
            {
                d0.push_back(parseAnnotation());
            }
            d.push_back({ca, d0});
        }
        attr = std::make_shared<OMClassRuntimeInvisibleParameterAnnotations>(n, d);
        break;
    }
    // RuntimeVisibleTypeAnnotations
    // RuntimeInvisibleTypeAnnotations
    case "AnnotationDefault"_hash: {
        attr = std::make_shared<OMClassAttrAnnotationDefault>(parseAnnotationValue());
        break;
    }
    case "BootstrapMethods"_hash: {
        uint16_t n;
        std::vector<OMClassBootMethods> data;
        this->source->readbe16(n);
        for (uint16_t i = 0; i < n; i++)
        {
            uint16_t ref, c;
            std::vector<uint16_t> d;
            this->source->readbe16(ref);
            this->source->readbe16(c);
            for (uint16_t j = 0; j < c; j++)
            {
                uint16_t di;
                this->source->readbe16(di);
                d.push_back(di);
            }
            data.push_back({ref, c, d});
        }
        attr = std::make_shared<OMClassAttrBootMethods>(n, data);
        break;
    }
    case "MethodParameters"_hash: {
        uint8_t pc;
        uint16_t a, b;
        std::vector<OMClassParam> d;
        this->source->read((char *)&pc, 1);
        for (uint8_t i = 0; i < pc; i++)
        {
            this->source->readbe16(a);
            this->source->readbe16(b);
            d.push_back({a, b});
        }
        attr = std::make_shared<OMClassAttrMethodParameters>(pc, d);
        break;
    }
    // Module
    case "ModulePackages"_hash: {
        uint16_t pc;
        std::vector<uint16_t> data;
        this->source->readbe16(pc);
        for (uint16_t i = 0; i < pc; i++)
        {
            uint16_t d;
            this->source->readbe16(d);
            data.push_back(d);
        }
        attr = std::make_shared<OMClassAttrModulePackages>(pc, data);
        break;
    }
    case "ModuleMainClass"_hash: {
        uint16_t mci;
        this->source->readbe16(mci);
        attr = std::make_shared<OMClassAttrModuleMainClass>(mci);
        break;
    }
    case "NestHost"_hash: {
        uint16_t d;
        this->source->readbe16(d);
        attr = std::make_shared<OMClassAttrNestHost>(d);
        break;
    }
    case "NestMembers"_hash: {
        uint16_t noc;
        std::vector<uint16_t> data;
        this->source->readbe16(noc);
        for (uint16_t i = 0; i < noc; i++)
        {
            uint16_t d;
            this->source->readbe16(d);
            data.push_back(d);
        }
        attr = std::make_shared<OMClassAttrNestMembers>(noc, data);
        break;
    }
    case "Record"_hash: {
        uint16_t c;
        this->source->readbe16(c);
        std::vector<OMClassRecordCompInfo> da;
        for (uint16_t i = 0; i < c; i++)
        {
            uint16_t ni, di, ac;
            std::vector<std::shared_ptr<OMClassAttr>> d;
            this->source->readbe16(ni);
            this->source->readbe16(di);
            this->source->readbe16(ac);
            for (uint16_t j = 0; j < ac; j++)
            {
                d.push_back(parseAttr(m));
            }
            da.push_back({ni, di, ac, d});
        }
        attr = std::make_shared<OMClassAttrRecord>(c, da);
        break;
    }
    case "PermittedSubclasses"_hash: {
        uint16_t noc;
        std::vector<uint16_t> data;
        this->source->readbe16(noc);
        for (uint16_t i = 0; i < noc; i++)
        {
            uint16_t d;
            this->source->readbe16(d);
            data.push_back(d);
        }
        attr = std::make_shared<OMClassAttrPermittedSubclasses>(noc, data);
        break;
    }
    default:
        this->source->seekg((uint64_t)this->source->tellg() + length);
        this->logger->info("Unimplemented attr: {}", m[ni]->to<OMClassConstantUtf8>()->data);
        break;
    }

    return attr;
}

std::shared_ptr<OMClassAnnotation> OMClassFileParser::parseAnnotation()
{
    this->source->seekg((uint64_t)this->source->tellg() + 2);
    auto anno = std::make_shared<OMClassAnnotation>();
    this->source->readbe16(anno->type);
    this->source->readbe16(anno->numPairs);
    anno->pairs = std::map<uint16_t, std::shared_ptr<OMClassAnnotationElemValue>>();

    for (uint16_t idx = 0; idx < anno->numPairs; idx++)
    {
        uint16_t i;
        this->source->readbe16(i);
        anno->pairs[i] = parseAnnotationValue();
    }

    return anno;
}

std::shared_ptr<OMClassAnnotationElemValue> OMClassFileParser::parseAnnotationValue()
{
    auto v = std::make_shared<OMClassAnnotationElemValue>();
    this->source->read((char *)&v->tag, 1);
    switch (v->tag)
    {
    case 'B':
    case 'C':
    case 'D':
    case 'F':
    case 'I':
    case 'J':
    case 'S':
    case 'Z':
    case 's':
        this->source->readbe16(v->constValueIndex);
        break;
    case 'e':
        this->source->readbe16(v->enumConstValue.typeNameIndex);
        this->source->readbe16(v->enumConstValue.constNameIndex);
        break;
    case 'c':
        this->source->readbe16(v->classInfoIndex);
        break;
    case '@':
        v->annotationValue = parseAnnotation();
        break;
    case '[': {
        std::vector<OMClassAnnotationElemValue> d;
        this->source->readbe16(v->arrayValue.numValues);
        for (uint16_t i = 0; i < v->arrayValue.numValues; i++)
        {
            d.push_back(*parseAnnotationValue());
        }
        v->arrayValue.values = d;
        break;
    }
    default:
        return nullptr;
    }
    return v;
}

std::string OMClassFileParser::toStdUtf8(std::vector<uint8_t> data, int length)
{
    int p = 0;
    std::vector<char> target;
    while (p < length)
    {
        if (data[p] >> 7 == 0)
        {
            target.push_back(data[p]);
            p += 1;
            continue;
        }

        if (data[p] >> 5 == 0b110 && data[p + 1] >> 6 == 0b10)
        {
            auto d = ((data[p] & 0x1f) << 6) + (data[p + 1] & 0x3f);
            if (d != 0)
                target.push_back(d);
            p += 2;
            continue;
        }

        if (data[p] >> 4 == 0b1110 && data[p + 1] >> 6 == 0b10 && data[p + 2] >> 5 == 0b10)
        {
            target.push_back(((data[p] & 0xf) << 12) + ((data[p + 1] & 0x3f) << 6) + (data[p + 2] & 0x3f));
            p += 3;
            continue;
        }

        if (data[p] == 0b11101101 && data[p + 1] >> 4 == 0b1010 && data[p + 2] >> 6 == 0b10 &&
            data[p + 3] == 0b11101101 && data[p + 4] >> 4 == 0b1011 && data[p + 5] >> 6 == 0b10)
        {
            target.push_back(0x10000 + ((data[p + 1] & 0x0f) << 16) + ((data[p + 2] & 0x3f) << 10) +
                             ((data[p + 4] & 0x0f) << 6) + (data[p + 5] & 0x3f));
            p += 6;
            continue;
        }
    }
    target.push_back('\0');

    return std::string(target.data());
}
} // namespace openminecraft::vm::classfile
