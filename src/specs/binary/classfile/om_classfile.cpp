#include "openminecraft/specs/classfile/om_classfile.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include <cstdint>
#include <iostream>
#include <istream>
#include <memory>
#include <stdexcept>

namespace openminecraft::specs::classfile
{
OMClassFile::OMClassFile() : logger("OMClassFile", this)
{
}

void OMClassFile::load(std::shared_ptr<std::istream> istr)
{
    istr->seekg(0, std::ios::end);
    auto l = istr->tellg();
    istr->seekg(0, std::ios::beg);

    std::vector<uint8_t> data;
    data.resize(l);
    istr->read(reinterpret_cast<char *>(data.data()), l);

    MemoryReader reader(data);

    header.magic = reader.readu32();
    header.minorVersion = reader.readu16();
    header.majorVersion = reader.readu16();

    if (header.magic != headerMagic)
    {
        throw std::logic_error("invaild header");
    }

    {
        constants.length = reader.readu16();

        constants.data = std::make_shared<std::vector<OMClassFileConstant>>();
        constants.data->resize(constants.length);
        auto c = constants.data->data();
        for (int i = 1; i < constants.length; ++i)
        {
            loadConstant(reader, c[i]);
            if (c[i].type == Long || c[i].type == Double)
            {
                ++i;
            }
        }
    }

    basic.accessFlags = reader.readu16();
    basic.thisClass = reader.readu16();
    basic.superClass = reader.readu16();

    {
        interfaces.length = reader.readu16();

        interfaces.data = std::make_shared<std::vector<uint16_t>>();
        interfaces.data->resize(interfaces.length);

        for (int i = 0; i < interfaces.length; ++i)
        {
            interfaces.data->at(i) = reader.readu16();
        }
    }

    {
        fields.length = reader.readu16();
        fields.data = std::make_shared<std::vector<OMClassField>>();
        fields.data->resize(fields.length);

        for (int i = 0; i < fields.length; ++i)
        {
            loadField(reader, fields.data->at(i));
        }
    }

    {
        methods.length = reader.readu16();
        methods.data = std::make_shared<std::vector<OMClassMethod>>();
        methods.data->resize(methods.length);

        for (int i = 0; i < methods.length; ++i)
        {
            loadMethod(reader, methods.data->at(i));
        }
    }

    {
        attributes.length = reader.readu16();
        attributes.data = std::make_shared<std::vector<OMClassAttribute>>();
        attributes.data->resize(attributes.length);

        for (int i = 0; i < attributes.length; ++i)
        {
            loadAttr(reader, attributes.data->at(i));
        }
    }
}

void OMClassFile::loadAttr(MemoryReader &reader, OMClassAttribute &a)
{
    using namespace binary::hash;
    a.nameIndex = reader.readu16();
    a.length = reader.readu32();
    auto name = constants.data->at(a.nameIndex).valueString;
    switch (binary::hash::hash_compile_time(name.c_str()))
    {
    case "ConstantValue"_hash: {
        a.constantValueIndex = reader.readu16();
        break;
    }
    case "Signature"_hash: {
        a.signatureIndex = reader.readu16();
        break;
    }
    case "Code"_hash: {
        a.code.maxStack = reader.readu16();
        a.code.maxLocal = reader.readu16();
        a.code.codeLength = reader.readu32();
        a.code.code = reinterpret_cast<uint8_t *>(std::malloc(a.code.codeLength));
        reader.readn(a.code.code, a.code.codeLength);
        a.code.exceptionTableLength = reader.readu16();

        a.code.exceptionTable = reinterpret_cast<OMClassExceptionTableEntry *>(
            std::malloc(a.code.exceptionTableLength * sizeof(OMClassExceptionTableEntry)));
        for (int i = 0; i < a.code.exceptionTableLength; ++i)
        {
            a.code.exceptionTable[i].start = reader.readu16();
            a.code.exceptionTable[i].end = reader.readu16();
            a.code.exceptionTable[i].handler = reader.readu16();
            a.code.exceptionTable[i].type = reader.readu16();
        }

        a.code.attrCount = reader.readu16();
        a.code.attrs = reinterpret_cast<OMClassAttribute *>(malloc(a.code.attrCount * sizeof(OMClassAttribute)));
        for (int i = 0; i < a.code.attrCount; ++i)
        {
            loadAttr(reader, a.code.attrs[i]);
        }
        break;
    }
    case "Exceptions"_hash: {
        a.exceptions.count = reader.readu16();

        a.exceptions.index = reinterpret_cast<uint16_t *>(std::malloc(2 * a.exceptions.count));
        for (int i = 0; i < a.exceptions.count; ++i)
        {
            a.exceptions.index[i] = reader.readu16();
        }
        break;
    }
    case "EnclosingMethod"_hash: {
        a.enclosingMethod.classIndex = reader.readu16();
        a.enclosingMethod.methodIndex = reader.readu16();
        break;
    }
    case "BootstrapMethods"_hash: {
        a.bootstrapMethod.numBootstrapMethods = reader.readu16();
        a.bootstrapMethod.bootstrapMethods = reinterpret_cast<OMClassBootstrapMethodEntry *>(
            malloc(a.bootstrapMethod.numBootstrapMethods * sizeof(OMClassBootstrapMethodEntry)));
        for (int i = 0; i < a.bootstrapMethod.numBootstrapMethods; ++i)
        {
            auto &mm = a.bootstrapMethod.bootstrapMethods[i];
            mm.bootstrapMethodRef = reader.readu16();
            mm.numBootstrapArguments = reader.readu16();
            mm.bootstrapArguments = reinterpret_cast<uint16_t *>(malloc(2 * mm.numBootstrapArguments));

            for (int j = 0; j < mm.numBootstrapArguments; ++j)
            {
                mm.bootstrapArguments[j] = reader.readu16();
            }
        }
        break;
    }
    default: {
        // logger.warn("skip {}", name);
        reader.skip(a.length);
    }
    }
}
void OMClassFile::loadMethod(MemoryReader &reader, OMClassMethod &m)
{
    m.accessFlags = reader.readu16();
    m.nameIndex = reader.readu16();
    m.descriptorIndex = reader.readu16();
    m.attributesCount = reader.readu16();

    m.attributes = std::make_shared<std::vector<OMClassAttribute>>();
    m.attributes->resize(m.attributesCount);
    for (int i = 0; i < m.attributesCount; ++i)
    {
        loadAttr(reader, m.attributes->at(i));
    }
}

void OMClassFile::loadField(MemoryReader &reader, OMClassField &m)
{
    m.accessFlags = reader.readu16();
    m.nameIndex = reader.readu16();
    m.descriptorIndex = reader.readu16();
    m.attributesCount = reader.readu16();

    m.attributes = std::make_shared<std::vector<OMClassAttribute>>();
    m.attributes->resize(m.attributesCount);
    for (int i = 0; i < m.attributesCount; ++i)
    {
        loadAttr(reader, m.attributes->at(i));
    }
}

static inline auto toStdUtf8(const uint8_t *data, int length) -> std::string
{
    std::string result;
    result.reserve(length);
    int p = 0;

    while (p < length)
    {
        uint8_t c = data[p];

        if (c < 0x80)
        {
            if (c == 0x00)
            {
                result.push_back('\0');
            }
            else
            {
                result.push_back(c);
            }
            p++;
        }
        else if (c == 0xC0 && p + 1 < length && data[p + 1] == 0x80)
        {
            result.push_back('\0');
            p += 2;
        }
        else if (c < 0xE0 && p + 1 < length)
        {
            result.push_back(c);
            result.push_back(data[p + 1]);
            p += 2;
        }
        else if (c < 0xF0 && p + 2 < length)
        {
            if (c == 0xED && (data[p + 1] & 0xF0) == 0xA0 && p + 5 < length && data[p + 3] == 0xED &&
                (data[p + 4] & 0xF0) == 0xB0)
            {
                uint32_t high = ((data[p + 1] & 0x0F) << 6) | (data[p + 2] & 0x3F);
                uint32_t low = ((data[p + 4] & 0x0F) << 6) | (data[p + 5] & 0x3F);
                uint32_t cp = 0x10000 + (high << 10) + low;
                result.push_back(0xF0 | (cp >> 18));
                result.push_back(0x80 | ((cp >> 12) & 0x3F));
                result.push_back(0x80 | ((cp >> 6) & 0x3F));
                result.push_back(0x80 | (cp & 0x3F));
                p += 6;
            }
            else
            {
                result.push_back(c);
                result.push_back(data[p + 1]);
                result.push_back(data[p + 2]);
                p += 3;
            }
        }
        else if (c < 0xF8 && p + 3 < length)
        {
            result.push_back(c);
            result.push_back(data[p + 1]);
            result.push_back(data[p + 2]);
            result.push_back(data[p + 3]);
            p += 4;
        }
        else
        {
            p++;
        }
    }

    return result;
}

void OMClassFile::loadConstant(MemoryReader &reader, OMClassFileConstant &c)
{
    c.type = (OMClassFileConstantType)reader.readu8();
    switch (c.type)
    {
    case Utf8: {
        auto l = reader.readu16();

        auto result = toStdUtf8(reader.raw(), l);
        reader.skip(l);

        c.valueString = result;

        break;
    }
    case Float:
    case Integer: {
        c.valueInteger = reader.readu32();
        break;
    }
    case Double:
    case Long: {
        c.valueLong = reader.readu64();
        break;
    }
    case Class: {
        c.classinfo.nameIndex = reader.readu16();
        break;
    }
    case String: {
        c.stringRef.stringIndex = reader.readu16();
        break;
    }
    case MethodRef:
    case FieldRef:
    case InterfaceMethodRef: {
        c.ref.classIndex = reader.readu16();
        c.ref.nameAndTypeIndex = reader.readu16();
        break;
    }
    case NameAndType: {
        c.nameAndType.nameIndex = reader.readu16();
        c.nameAndType.descriptorIndex = reader.readu16();
        break;
    }
    case MethodHandle: {
        c.methodHandle.refKind = static_cast<OMClassRefKind>(reader.readu8());
        c.methodHandle.refIndex = reader.readu16();
        break;
    }
    case MethodType: {
        c.methodType.descriptorIndex = reader.readu16();
        break;
    }
    case InvokeDynamic:
    case Dynamic: {
        c.dynamic.bootstrapIndex = reader.readu16();
        c.dynamic.nameAndTypeIndex = reader.readu16();
        break;
    }
    case Module: {
        c.module.nameIndex = reader.readu16();
        break;
    }
    case Package: {
        c.package.nameIndex = reader.readu16();
        break;
    }
    default: {
        throw std::logic_error("unknown constant type");
    }
    }
}
} // namespace openminecraft::specs::classfile
