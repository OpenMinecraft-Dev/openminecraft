#include "openminecraft/log/om_log_common.hpp"
#include <cstdint>
#include <fmt/format.h>
#include <openminecraft/vm/om_class_file.hpp>
#include <openminecraft/binary/om_bin_endians.hpp>
#include <stdexcept>
#include <vector>

using namespace openminecraft::binary;

namespace openminecraft::vm::classfile
{
    OMClassConstantFieldRef::OMClassConstantFieldRef(uint16_t ci, uint16_t nti): classIndex(ci), nameAndTypeIndex(nti) {}
    OMClassConstantType OMClassConstantFieldRef::type() { return OMClassConstantType::FieldRef; }
    OMClassConstantMethodRef::OMClassConstantMethodRef(uint16_t ci, uint16_t nti): classIndex(ci), nameAndTypeIndex(nti) {}
    OMClassConstantType OMClassConstantMethodRef::type() { return OMClassConstantType::MethodRef; }
    OMClassConstantInterfaceMethodRef::OMClassConstantInterfaceMethodRef(uint16_t ci, uint16_t nti): classIndex(ci), nameAndTypeIndex(nti) {}
    OMClassConstantType OMClassConstantInterfaceMethodRef::type() { return OMClassConstantType::InterfaceMethodRef; }
    OMClassConstantClass::OMClassConstantClass(uint16_t ni): nameIndex(ni) {}
    OMClassConstantType OMClassConstantClass::type() { return OMClassConstantType::Class; }
    OMClassConstantNameAndType::OMClassConstantNameAndType(uint16_t ni, uint16_t di): nameIndex(ni), descIndex(di) {}
    OMClassConstantType OMClassConstantNameAndType::type() { return OMClassConstantType::NameAndType; }
    OMClassConstantUtf8::OMClassConstantUtf8(std::string data): data(data) {}
    OMClassConstantType OMClassConstantUtf8::type() { return OMClassConstantType::Utf8; }
    OMClassConstantString::OMClassConstantString(uint16_t si): stringIndex(si) {}
    OMClassConstantType OMClassConstantString::type() { return OMClassConstantType::String; }
    OMClassConstantInteger::OMClassConstantInteger(int data): data(data) {}
    OMClassConstantType OMClassConstantInteger::type() { return OMClassConstantType::Integer; }
    OMClassConstantFloat::OMClassConstantFloat(float data): data(data) {}
    OMClassConstantType OMClassConstantFloat::type() { return OMClassConstantType::Float; }
    OMClassConstantLong::OMClassConstantLong(int64_t data): data(data) {}
    OMClassConstantType OMClassConstantLong::type() { return OMClassConstantType::Long; }
    OMClassConstantDouble::OMClassConstantDouble(double data): data(data) {}
    OMClassConstantType OMClassConstantDouble::type() { return OMClassConstantType::Double; }
    OMClassConstantMethodHandle::OMClassConstantMethodHandle(uint8_t rk, uint16_t ri): refKind(rk), refIndex(ri) {}
    OMClassConstantType OMClassConstantMethodHandle::type() { return OMClassConstantType::MethodHandle; }
    OMClassConstantMethodType::OMClassConstantMethodType(uint16_t di): descIndex(di) {}
    OMClassConstantType OMClassConstantMethodType::type() { return OMClassConstantType::MethodType; }
    OMClassConstantDynamic::OMClassConstantDynamic(uint16_t bmai, uint16_t nti): bootstrapMethodAttrIndex(bmai), nameAndTypeIndex(nti) {}
    OMClassConstantType OMClassConstantDynamic::type() { return OMClassConstantType::Dynamic; }
    OMClassConstantInvokeDynamic::OMClassConstantInvokeDynamic(uint16_t bmai, uint16_t nti): bootstrapMethodAttrIndex(bmai), nameAndTypeIndex(nti) {}
    OMClassConstantType OMClassConstantInvokeDynamic::type() { return OMClassConstantType::InvokeDynamic; }
    OMClassConstantModule::OMClassConstantModule(uint16_t ni): nameIndex(ni) {}
    OMClassConstantType OMClassConstantModule::type() { return OMClassConstantType::Module; }
    OMClassConstantPackage::OMClassConstantPackage(uint16_t ni): nameIndex(ni) {}
    OMClassConstantType OMClassConstantPackage::type() { return OMClassConstantType::Package; }

    OMClassFileParser::OMClassFileParser(std::istream& str)
    {
        this->source = &str;
        this->logger = new log::OMLogger("OMClassFileParser", this);
    }

    OMClassFileParser::~OMClassFileParser()
    {
        this->logger->info("Destroying class file parser");
        delete this->source;
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
        while (idx < file->constantPoolCount - 1)
        {
            file->constants.push_back(this->parseConstant(&idx));
        }

        this->source->readbe16(file->accessFlags);
        this->source->readbe16(file->thisClass);
        this->source->readbe16(file->superClass);
        this->source->readbe16(file->interfacesCount);

        return file;
    }

    OMClassConstant* OMClassFileParser::parseConstant(uint16_t* idx)
    {
        (*idx)++;

        OMClassConstantType type;
        this->source->read((char*) &type, 1);

        uint16_t temp1, temp2, temp3, temp4;
        uint32_t temp5, temp6;

        OMClassConstant* result = nullptr;
        switch (type) {
            case OMClassConstantType::Class:
            {
                this->source->readbe16(temp1);
                result = new OMClassConstantClass(temp1);
                break;
            }
            case OMClassConstantType::FieldRef:
            {
                this->source->readbe16(temp1);
                this->source->readbe16(temp2);
                result = new OMClassConstantFieldRef(temp1, temp2);
                break;
            }
            case OMClassConstantType::MethodRef:
            {
                this->source->readbe16(temp1);
                this->source->readbe16(temp2);
                result = new OMClassConstantMethodRef(temp1, temp2);
                break;
            }
            case OMClassConstantType::InterfaceMethodRef:
            {
                this->source->readbe16(temp1);
                this->source->readbe16(temp2);
                result = new OMClassConstantInterfaceMethodRef(temp1, temp2);
                break;
            }
            case OMClassConstantType::NameAndType:
            {
                this->source->readbe16(temp1);
                this->source->readbe16(temp2);
                result = new OMClassConstantNameAndType(temp1, temp2);
                break;
            }
            case OMClassConstantType::Utf8:
            {
                this->source->readbe16(temp1);
                auto temp = new uint8_t[temp1];
                this->source->read((char*) temp, temp1);
                auto comp = std::string(toStdUtf8(temp, temp1));
                delete[] temp;
                result = new OMClassConstantUtf8(comp);
                break;
            }
            case OMClassConstantType::String:
            {
                this->source->readbe16(temp1);
                result = new OMClassConstantString(temp1);
                break;
            }
            case OMClassConstantType::Integer:
            {
                this->source->readbe32(temp5);
                union
                {
                    uint32_t uidata;
                    int idata;
                } d;
                d.uidata = temp5;
                result = new OMClassConstantInteger(d.idata);
                break;
            }
            case OMClassConstantType::Float:
            {
                this->source->readbe32(temp5);
                union
                {
                    uint32_t idata;
                    float fdata;
                } d;
                d.idata = temp5;
                result = new OMClassConstantFloat(d.fdata);
                break;
            }
            case OMClassConstantType::Long:
            {
                this->source->readbe32(temp5);
                this->source->readbe32(temp6);
                result = new OMClassConstantLong(((int64_t) temp5 << 32) + temp6);
                (*idx)++;
                break;
            }
            case OMClassConstantType::Double:
            {
                this->source->readbe32(temp5);
                this->source->readbe32(temp6);
                union
                {
                    int64_t idata;
                    double ddata;
                } d;
                d.idata = ((int64_t) temp5 << 32) + temp6;
                result = new OMClassConstantDouble(d.ddata);
                (*idx)++;
                break;
            }
            case OMClassConstantType::MethodHandle:
            {
                uint8_t temp;
                this->source->read((char*) &temp, 1);
                this->source->readbe16(temp1);
                result = new OMClassConstantMethodHandle(temp, temp1);
                break;
            }
            case OMClassConstantType::MethodType:
            {
                this->source->readbe16(temp1);
                result = new OMClassConstantMethodType(temp1);
                break;
            }
            case OMClassConstantType::Dynamic:
            {
                this->source->readbe16(temp1);
                this->source->readbe16(temp2);
                result = new OMClassConstantDynamic(temp1, temp2);
                break;
            }
            case OMClassConstantType::InvokeDynamic:
            {
                this->source->readbe16(temp1);
                this->source->readbe16(temp2);
                result = new OMClassConstantInvokeDynamic(temp1, temp2);
                break;
            }
            case OMClassConstantType::Module:
            {
                this->source->readbe16(temp1);
                result = new OMClassConstantModule(temp1);
                break;
            }
            case OMClassConstantType::Package:
            {
                this->source->readbe16(temp1);
                result = new OMClassConstantPackage(temp1);
                break;
            }
            default:
            {
                throw std::invalid_argument("Unknown constant type id!");
                break;
            }
        }

        return result;
    }

    char* OMClassFileParser::toStdUtf8(uint8_t* data, int length)
    {
        int p = 0;
        auto target = new std::vector<char>();
        while (p < length)
        {
            if (data[p] >> 7 == 0)
            {
                target->push_back(data[p]);
                p += 1;
                continue;
            }

            if (data[p] >> 5 == 0b110 && data[p + 1] >> 6 == 0b10)
            {
                auto d = ((data[p] & 0x1f) << 6) + (data[p + 1] & 0x3f);
                if (d != 0) target->push_back(d);
                p += 2;
                continue;
            }

            if (data[p] >> 4 == 0b1110 && data[p + 1] >> 6 == 0b10 && data[p + 2] >> 5 == 0b10)
            {
                target->push_back(((data[p] & 0xf) << 12) + ((data[p + 1] & 0x3f) << 6) + (data[p + 2] & 0x3f));
                p += 3;
                continue;
            }

            if (data[p] == 0b11101101 && data[p + 1] >> 4 == 0b1010 && data[p + 2] >> 6 == 0b10 && data[p + 3] == 0b11101101 && data[p + 4] >> 4 == 0b1011 && data[p + 5] >> 6 == 0b10)
            {
                target->push_back(0x10000 + ((data[p + 1] & 0x0f) << 16) + ((data[p + 2] & 0x3f) << 10) +
                ((data[p + 4] & 0x0f) << 6) + (data[p + 5] & 0x3f));
                p += 6;
                continue;
            }
        }
        target->push_back('\0');

        return target->data();
    }
}