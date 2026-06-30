#include "openminecraft/specs/classfile/om_classfile.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/util/om_util_encoding_utf.hpp"
#include <cstdint>
#include <istream>
#include <memory>
#include <stdexcept>

namespace openminecraft::specs::classfile
{
OMClassFile::OMClassFile() : logger("OMClassFile", this)
{
}

OMClassFile::~OMClassFile()
{
}

void OMClassFile::load(std::shared_ptr<std::istream> istr)
{
    istr->read(reinterpret_cast<char *>(&header), sizeof(header));
    header.minorVersion = binary::be16ToNative(header.minorVersion);
    header.majorVersion = binary::be16ToNative(header.majorVersion);

    if (std::memcmp(header.magic, headerMagic, 4))
    {
        throw std::logic_error("invaild header");
    }

    {
        istr->read(reinterpret_cast<char *>(&constants.length), 2);
        constants.length = binary::be16ToNative(constants.length);

        constants.data = std::shared_ptr<OMClassFileConstant[]>(new OMClassFileConstant[constants.length],
                                                                [](OMClassFileConstant *p) { delete[] p; });
        auto c = constants.data.get();
        for (int i = 1; i < constants.length; ++i)
        {
            loadConstant(istr, c[i]);
            if (c[i].type == Long || c[i].type == Double)
            {
                ++i;
            }
        }
    }

    istr->read(reinterpret_cast<char *>(&basic), sizeof(basic));
    basic.accessFlags = binary::be16ToNative(basic.accessFlags);
    basic.thisClass = binary::be16ToNative(basic.thisClass);
    basic.superClass = binary::be16ToNative(basic.superClass);

    {
        istr->read(reinterpret_cast<char *>(&interfaces.length), 2);
        interfaces.length = binary::be16ToNative(interfaces.length);

        interfaces.data = std::shared_ptr<uint16_t[]>(new uint16_t[interfaces.length], [](uint16_t *p) { delete[] p; });

        istr->read(reinterpret_cast<char *>(interfaces.data.get()), 2 * interfaces.length);
        for (int i = 0; i < interfaces.length; ++i)
        {
            interfaces.data[i] = binary::be16ToNative(interfaces.data[i]);
        }
    }

    {
        istr->read(reinterpret_cast<char *>(&fields.length), 2);
        fields.length = binary::be16ToNative(fields.length);
        fields.data =
            std::shared_ptr<OMClassField[]>(new OMClassField[fields.length], [](OMClassField *p) { delete[] p; });

        for (int i = 0; i < fields.length; ++i)
        {
            loadField(istr, fields.data[i]);
        }
    }
}

void OMClassFile::loadAttr(std::shared_ptr<std::istream> istr, OMClassAttribute &a)
{
    using namespace binary::hash;
    istr->read(reinterpret_cast<char *>(&a.nameIndex), 2);
    a.nameIndex = binary::be16ToNative(a.nameIndex);
    istr->read(reinterpret_cast<char *>(&a.length), 4);
    a.length = binary::be32ToNative(a.length);
    auto name = constants.data[a.nameIndex].valueString;
    switch (binary::hash::hash_compile_time(name))
    {
    case "ConstantValue"_hash: {
        istr->read(reinterpret_cast<char *>(&a.constantValueIndex), 2);
        a.constantValueIndex = binary::be16ToNative(a.constantValueIndex);
        break;
    }
    case "Signature"_hash: {
        istr->read(reinterpret_cast<char *>(&a.signatureIndex), 2);
        a.signatureIndex = binary::be16ToNative(a.signatureIndex);
        break;
    }
    default: {
        logger.warn("skip {}", name);
        istr->seekg(a.length, std::ios::cur);
    }
    }
}

void OMClassFile::loadField(std::shared_ptr<std::istream> istr, OMClassField &f)
{
    istr->read(reinterpret_cast<char *>(&f.accessFlags), 2);
    f.accessFlags = binary::be16ToNative(f.accessFlags);
    istr->read(reinterpret_cast<char *>(&f.nameIndex), 2);
    f.nameIndex = binary::be16ToNative(f.nameIndex);
    istr->read(reinterpret_cast<char *>(&f.descriptorIndex), 2);
    f.descriptorIndex = binary::be16ToNative(f.descriptorIndex);
    istr->read(reinterpret_cast<char *>(&f.attributesCount), 2);
    f.attributesCount = binary::be16ToNative(f.attributesCount);

    f.attributes = std::shared_ptr<OMClassAttribute[]>(new OMClassAttribute[f.attributesCount],
                                                       [](OMClassAttribute *p) { delete[] p; });
    for (int i = 0; i < f.attributesCount; ++i)
    {
        loadAttr(istr, f.attributes[i]);
    }
}

static inline std::string toStdUtf8(uint8_t *data, int length)
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
            target.push_back(d);
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

    return util::encoding::utf32ToUtf8(target);
}

void OMClassFile::loadConstant(std::shared_ptr<std::istream> istr, OMClassFileConstant &c)
{
    istr->read(reinterpret_cast<char *>(&c.type), 1);
    switch (c.type)
    {
    case Utf8: {
        uint16_t l;
        istr->read(reinterpret_cast<char *>(&l), 2);
        l = binary::be16ToNative(l);
        auto arr = new char[l];
        istr->read(arr, l);
        auto result = toStdUtf8((uint8_t *)arr, l);
        delete[] arr;

        c.valueString = (char *)mem::allocator::tracedCallocSpecs(l + 1, 1);
        std::strcpy(c.valueString, result.data());

        break;
    }
    case Integer: {
        istr->read(reinterpret_cast<char *>(&c.valueInteger), 4);
        c.valueInteger = binary::be32ToNative(c.valueInteger);
        break;
    }
    case Long: {
        istr->read(reinterpret_cast<char *>(&c.valueLong), 8);
        c.valueLong = binary::be64ToNative(c.valueLong);
        break;
    }
    case Class: {
        istr->read(reinterpret_cast<char *>(&c.classinfo.nameIndex), 2);
        c.classinfo.nameIndex = binary::be16ToNative(c.classinfo.nameIndex);
        break;
    }
    case String: {
        istr->read(reinterpret_cast<char *>(&c.stringRef.stringIndex), 2);
        c.stringRef.stringIndex = binary::be16ToNative(c.stringRef.stringIndex);
        break;
    }
    case MethodRef:
    case FieldRef:
    case InterfaceMethodRef: {
        istr->read(reinterpret_cast<char *>(&c.ref), sizeof(c.ref));
        c.ref.classIndex = binary::be16ToNative(c.ref.classIndex);
        c.ref.nameAndTypeIndex = binary::be16ToNative(c.ref.nameAndTypeIndex);
        break;
    }
    case NameAndType: {
        istr->read(reinterpret_cast<char *>(&c.nameAndType), sizeof(c.nameAndType));
        c.nameAndType.nameIndex = binary::be16ToNative(c.nameAndType.nameIndex);
        c.nameAndType.descriptorIndex = binary::be16ToNative(c.nameAndType.descriptorIndex);
        break;
    }
    case MethodHandle: {
        istr->read(reinterpret_cast<char *>(&c.methodHandle.refKind), 1);
        istr->read(reinterpret_cast<char *>(&c.methodHandle.refIndex), 2);
        c.methodHandle.refIndex = binary::be16ToNative(c.methodHandle.refIndex);
        break;
    }
    case MethodType: {
        istr->read(reinterpret_cast<char *>(&c.methodType.descriptorIndex), 2);
        c.methodType.descriptorIndex = binary::be16ToNative(c.methodType.descriptorIndex);
        break;
    }
    case InvokeDynamic:
    case Dynamic: {
        istr->read(reinterpret_cast<char *>(&c.dynamic), sizeof(c.dynamic));
        c.dynamic.bootstrapIndex = binary::be16ToNative(c.dynamic.bootstrapIndex);
        c.dynamic.nameAndTypeIndex = binary::be16ToNative(c.dynamic.nameAndTypeIndex);
        break;
    }
    case Module: {
        istr->read(reinterpret_cast<char *>(&c.module.nameIndex), 2);
        c.module.nameIndex = binary::be16ToNative(c.module.nameIndex);
        break;
    }
    case Package: {
        istr->read(reinterpret_cast<char *>(&c.package.nameIndex), 2);
        c.package.nameIndex = binary::be16ToNative(c.package.nameIndex);
        break;
    }
    default: {
        throw std::logic_error("unknown constant type");
    }
    }
}
} // namespace openminecraft::specs::classfile
