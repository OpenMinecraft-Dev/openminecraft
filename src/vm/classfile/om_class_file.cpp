#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/io/om_io_parser.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/encoding/om_encoding_utf.hpp"

#include <cstdint>
#include <istream>
#include <fmt/format.h>
#include <memory>
#include <openminecraft/binary/om_bin_endians.hpp>
#include <openminecraft/vm/classfile/om_class_file.hpp>

using namespace openminecraft::binary;
using namespace openminecraft::binary::hash;
using namespace openminecraft::util;

namespace openminecraft::vm::classfile
{
OMClassFileParser::OMClassFileParser(std::istream *str) : io::OMParser(str)
{
    this->logger = std::make_shared<log::OMLogger>("OMClassFileParser", this);
}

OMClassFileParser::~OMClassFileParser()
{
    io::OMParser::~OMParser();
}

util::OMResult<std::shared_ptr<OMClassFile>, err::OMValidationError> OMClassFileParser::parse()
{
    auto file = std::make_shared<OMClassFile>();
    this->source->readbe32(file->magicNumber);
    if (file->magicNumber != 0xcafebabe)
    {
        return OMResult<std::shared_ptr<OMClassFile>, err::OMValidationError>::err(
            err::OMValidationError(err::ValidationState::Loading, "invalid class file magic number", ""));
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
            return OMResult<std::shared_ptr<OMClassFile>, err::OMValidationError>::err(c.unwrap_err());
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

    file->mapping = m;

    return OMResult<std::shared_ptr<OMClassFile>, err::OMValidationError>::ok(file);
}

OMResult<std::shared_ptr<OMClassConstant>, err::OMValidationError> OMClassFileParser::parseConstant(uint16_t *idx) const
{
    (*idx)++;

    OMClassConstantType type;
    this->source->read((char *)&type, 1);

    uint16_t temp1, temp2;
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
        return OMResult<std::shared_ptr<OMClassConstant>, err::OMValidationError>::err(err::OMValidationError(
            err::ValidationState::Loading, "unknown constant type id", fmt::format("{} at index {}", (int)type, *idx)));
    }
    }

    return OMResult<std::shared_ptr<OMClassConstant>, err::OMValidationError>::ok(result);
}

OMClassFileParser::ConstantMapping OMClassFileParser::buildConstantMapping(
    const std::vector<std::shared_ptr<OMClassConstant>> &c)
{
    OMClassFileParser::ConstantMapping target;
    uint16_t id = 1;
    for (const auto &d : c)
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

std::shared_ptr<OMClassFieldInfo> OMClassFileParser::parseField(const OMClassFileParser::ConstantMapping &m)
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

std::shared_ptr<OMClassMethodInfo> OMClassFileParser::parseMethod(const OMClassFileParser::ConstantMapping &m)
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
        return nullptr;
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
        auto code = std::make_shared<std::vector<uint8_t>>(cl);
        this->source->read((char *)code->data(), cl);
        this->source->readbe16(etl);
        std::vector<OMClassAttrCodeExcTable> et;
        for (uint16_t i = 0; i < etl; i++)
        {
            uint16_t sp, ep, hp, ct;
            this->source->readbe16(sp);
            this->source->readbe16(ep);
            this->source->readbe16(hp);
            this->source->readbe16(ct);
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
            OMClassAttrVerifyTypeInfo s{};
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
            this->source->read(reinterpret_cast<char *>(&fr->tag), 1);
            if (fr->tag < 64)
            {
            }
            else if (fr->tag >= 64 && fr->tag < 128)
            {
                fr->sameLocals1StackItemFrame.stack = typep();
            }
            else if (fr->tag < 247)
            {
                return nullptr;
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
                fr->appendFrame.locals = new std::vector<OMClassAttrVerifyTypeInfo>();
                for (uint8_t idx = 0; idx < fr->tag - 251; idx++)
                {
                    fr->appendFrame.locals->push_back(typep());
                }
            }
            else
            {
                this->source->readbe16(fr->fullFrame.offset);
                this->source->readbe16(fr->fullFrame.numberOfLocals);
                fr->fullFrame.locals = new std::vector<OMClassAttrVerifyTypeInfo>();
                for (uint16_t idxx = 0; idxx < fr->fullFrame.numberOfLocals; idxx++)
                {
                    fr->fullFrame.locals->push_back(typep());
                }
                this->source->readbe16(fr->fullFrame.numberOfStackItems);
                fr->fullFrame.stackItems = new std::vector<OMClassAttrVerifyTypeInfo>();
                for (uint16_t idxx = 0; idxx < fr->fullFrame.numberOfStackItems; idxx++)
                {
                    fr->fullFrame.stackItems->push_back(typep());
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
            OMClassAttrInnerClassInfo di{};
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
        std::unordered_map<uint16_t, uint16_t> lnt;
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
            OMClassAttrLocalVar data{};
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
            OMClassAttrLocalVar data{};
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
        attr = std::make_shared<OMClassAttrRuntimeVisibleParameterAnnotations>(n, d);
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
        attr = std::make_shared<OMClassAttrRuntimeInvisibleParameterAnnotations>(n, d);
        break;
    }
    case "RuntimeVisibleTypeAnnotations"_hash: {
        uint16_t n = 0;
        std::vector<std::shared_ptr<OMClassRuntimeTypeAnnotation>> data;
        this->source->readbe16(n);
        for (int i = 0; i < n; i++)
        {
            data.push_back(parseTypeAnnotation());
        }
        attr = std::make_shared<OMClassAttrRuntimeVisibleTypeAnnotation>(n, data);
        break;
    }
    case "RuntimeInvisibleTypeAnnotations"_hash: {
        uint16_t n = 0;
        std::vector<std::shared_ptr<OMClassRuntimeTypeAnnotation>> data;
        this->source->readbe16(n);
        for (int i = 0; i < n; i++)
        {
            data.push_back(parseTypeAnnotation());
        }
        attr = std::make_shared<OMClassAttrRuntimeInvisibleTypeAnnotation>(n, data);
        break;
    }
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
    case "Module"_hash: {
        uint16_t mni;
        uint16_t mf;
        uint16_t mvi;
        uint16_t rc;
        std::vector<OMClassModuleRequire> r;
        uint16_t ec;
        std::vector<OMClassModuleExport> e;
        uint16_t oc;
        std::vector<OMClassModuleOpen> o;
        uint16_t uc;
        std::vector<uint16_t> u;
        uint16_t pc;
        std::vector<OMClassModuleProvide> p;

        this->source->readbe16(mni);
        this->source->readbe16(mf);
        this->source->readbe16(mvi);
        this->source->readbe16(rc);
        for (int i = 0; i < rc; i++)
        {
            OMClassModuleRequire req;

            this->source->readbe16(req.requireIndex);
            this->source->readbe16(req.requireFlags);
            this->source->readbe16(req.requireVersionIndex);

            r.emplace_back(req);
        }

        this->source->readbe16(ec);
        for (int i = 0; i < ec; i++)
        {
            OMClassModuleExport exp;

            this->source->readbe16(exp.exportIndex);
            this->source->readbe16(exp.exportFlags);
            this->source->readbe16(exp.exportToCount);

            for (int j = 0; j < exp.exportToCount; j++)
            {
                uint16_t l;
                this->source->readbe16(l);
                exp.exportToIndex.push_back(l);
            }

            e.emplace_back(exp);
        }

        this->source->readbe16(oc);
        for (int i = 0; i < oc; i++)
        {
            OMClassModuleOpen op;

            this->source->readbe16(op.openIndex);
            this->source->readbe16(op.openFlags);
            this->source->readbe16(op.openToCount);

            for (int j = 0; j < op.openToCount; j++)
            {
                uint16_t l;
                this->source->readbe16(l);
                op.openToIndex.push_back(l);
            }

            o.emplace_back(op);
        }

        this->source->readbe16(uc);
        for (int i = 0; i < uc; i++)
        {
            uint16_t l;
            this->source->readbe16(l);
            u.push_back(l);
        }

        this->source->readbe16(pc);
        for (int i = 0; i < pc; i++)
        {
            OMClassModuleProvide pv;

            this->source->readbe16(pv.provideIndex);
            this->source->readbe16(pv.provideWithCount);

            for (int j = 0; j < pv.provideWithCount; j++)
            {
                uint16_t l;
                this->source->readbe16(l);
                pv.provideWithIndex.push_back(l);
            }
        }

        attr = std::make_shared<OMClassAttrModule>(mni, mf, mvi, rc, r, ec, e, oc, o, uc, u, pc, p);

        break;
    }
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
            uint16_t nib, di, ac;
            std::vector<std::shared_ptr<OMClassAttr>> d;
            this->source->readbe16(nib);
            this->source->readbe16(di);
            this->source->readbe16(ac);
            for (uint16_t j = 0; j < ac; j++)
            {
                d.push_back(parseAttr(m));
            }
            da.push_back({nib, di, ac, d});
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
        this->source->seekg(static_cast<int64_t>(this->source->tellg()) + length);
        this->logger->warn("Unimplemented attr: {}", m[ni]->to<OMClassConstantUtf8>()->data);
        break;
    }

    return attr;
}

std::shared_ptr<OMClassRuntimeTypeAnnotation> OMClassFileParser::parseTypeAnnotation()
{
    auto result = std::make_shared<OMClassRuntimeTypeAnnotation>();

    this->source->read(reinterpret_cast<char *>(&result->targetType), 1);

    switch (result->targetType)
    {
    case 0x00:
    case 0x01: {
        this->source->read(reinterpret_cast<char *>(&result->targetInfo.typeParameter.typeParamIndex), 1);
        break;
    }
    case 0x10: {
        this->source->readbe16(result->targetInfo.supertype.supertypeIndex);
        break;
    }
    case 0x11:
    case 0x12: {
        this->source->read(reinterpret_cast<char *>(&result->targetInfo.typeParameterBound.typeParamIndex), 1);
        this->source->read(reinterpret_cast<char *>(&result->targetInfo.typeParameterBound.boundIndex), 1);
        break;
    }
    case 0x13:
    case 0x14:
    case 0x15: {
        break;
    }
    case 0x16: {
        this->source->read(reinterpret_cast<char *>(&result->targetInfo.formalParameter.formalParamIndex), 1);
        break;
    }
    case 0x17: {
        this->source->readbe16(result->targetInfo.throws.throwsTypeIndex);
        break;
    }
    case 0x40:
    case 0x41: {
        this->source->readbe16(result->targetInfo.localVar.tableLength);
        for (int i = 0; i < result->targetInfo.localVar.tableLength; i++)
        {
            OMClassRuntimeTypeTargetTableItem item;

            this->source->read(reinterpret_cast<char *>(&item.startPc), 1);
            this->source->read(reinterpret_cast<char *>(&item.length), 1);
            this->source->read(reinterpret_cast<char *>(&item.index), 1);

            result->targetInfo.localVar.table.emplace_back(item);
        }
        break;
    }
    case 0x42: {
        this->source->readbe16(result->targetInfo.catches.exceptionTableIndex);
        break;
    }
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46: {
        this->source->readbe16(result->targetInfo.offset.offset);
        break;
    }
    case 0x47:
    case 0x48:
    case 0x49:
    case 0x4a:
    case 0x4b: {
        this->source->readbe16(result->targetInfo.typeArgument.offset);
        this->source->read(reinterpret_cast<char *>(&result->targetInfo.typeArgument.typeArgumentIndex), 1);
        break;
    }
    default: {
        break;
    }
    }

    this->source->read(reinterpret_cast<char *>(&result->targetPath.length), 1);
    for (int i = 0; i < result->targetPath.length; i++)
    {
        OMClassRuntimeTypePath pth;

        this->source->read(reinterpret_cast<char *>(&pth.typePathKind), 1);
        this->source->read(reinterpret_cast<char *>(&pth.typeArgumentIndex), 1);

        result->targetPath.paths.emplace_back(pth);
    }

    this->source->readbe16(result->typeIndex);
    this->source->readbe16(result->numEnumValuePairs);

    for (int i = 0; i < result->numEnumValuePairs; i++)
    {
        OMClassRuntimeTypeElementValue v;

        this->source->readbe16(v.elementNameIndex);
        v.value = parseAnnotationValue();

        result->enumValuePairs.emplace_back(v);
    }

    return result;
}

std::shared_ptr<OMClassAnnotation> OMClassFileParser::parseAnnotation()
{
    auto anno = std::make_shared<OMClassAnnotation>();
    this->source->readbe16(anno->type);
    this->source->readbe16(anno->numPairs);
    anno->pairs = std::unordered_map<uint16_t, std::shared_ptr<OMClassAnnotationElemValue>>();

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

    std::vector<int> target;
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
            {
                target.push_back(d);
            }
            p += 2;
            continue;
        }

        if (data[p] >> 4 == 0b1110 && data[p + 1] >> 6 == 0b10 && data[p + 2] >> 6 == 0b10)
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

    return encoding::utf32ToUtf8(target);
}
} // namespace openminecraft::vm::classfile
