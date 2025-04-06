#include "openminecraft/log/om_log_common.hpp"
#include <openminecraft/vm/om_class_file.hpp>
#include <openminecraft/binary/om_bin_endians.hpp>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace openminecraft::binary;

namespace openminecraft::vm::classfile
{
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
        file->constants = std::vector<OMClassConstantItem>(file->constantPoolCount - 1);

        for (int i = 0; i < file->constantPoolCount - 1; i++)
        {
            file->constants[i] = parseConstant(i + 1);
            uint8_t id = *file->constants[i];
            if (id == JVM_Constant_Long || id == JVM_Constant_Double) i++;
        }

        return file;
    }

    OMClassConstantItem OMClassFileParser::parseConstant(int index)
    {
        uint8_t id;
        this->source->read((char*) &id, 1);
        switch (id)
        {
            case JVM_Constant_Utf8:
                return parseConstantUtf8(index, id);
            case JVM_Constant_Integer:
                return parseConstantInteger(index, id);
            case JVM_Constant_Float:
                return parseConstantFloat(index, id);
            case JVM_Constant_Long:
                return parseConstantLong(index, id);
            case JVM_Constant_Double:
                return parseConstantDouble(index, id);
            case JVM_Constant_Class:
                return parseConstantClass(index, id);
            case JVM_Constant_String:
                return parseConstantString(index, id);
            case JVM_Constant_FieldRef:
            case JVM_Constant_MethodRef:
            case JVM_Constant_InterfaceMethodRef:
                return parseConstantRef(index, id);
            case JVM_Constant_NameAndType:
                return parseConstantNameAndType(index, id);
        }

        std::stringstream s;
        s << "Unknown constant pool id " << (int) id << "!";
        throw std::invalid_argument(s.str());
    }

    OMClassConstantItem OMClassFileParser::parseConstantClass(int index, uint8_t id)
    {
        auto cls = new OMClassConstantClass;
        cls->type = id;
        this->source->readbe16(cls->nameIndex);
        omLog(logger->info, "Class #" << index << ": #" << cls->nameIndex);
        return (OMClassConstantItem) cls;
    }

    OMClassConstantItem OMClassFileParser::parseConstantRef(int index, uint8_t id)
    {
        auto ref = new OMClassConstantRef;
        ref->type = id;
        this->source->readbe16(ref->classIndex);
        this->source->readbe16(ref->nameAndTypeIndex);
        omLog(logger->info, "Ref (0x" << std::hex << (int) ref->type << std::dec << ") #" << index << ": #" << ref->classIndex << ".#" << ref->nameAndTypeIndex);
        return (OMClassConstantItem) ref;
    }

    OMClassConstantItem OMClassFileParser::parseConstantNameAndType(int index, uint8_t id)
    {
        auto ref = new OMClassConstantNameAndType;
        ref->type = id;
        this->source->readbe16(ref->nameIndex);
        this->source->readbe16(ref->descriptorIndex);
        omLog(logger->info, "NameAndType #" << index << ": #" << ref->nameIndex << ".#" << ref->descriptorIndex);
        return (OMClassConstantItem) ref;
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

    OMClassConstantItem OMClassFileParser::parseConstantUtf8(int index, uint8_t id)
    {
        auto ref = new OMClassConstantUtf8;
        ref->type = id;

        uint16_t length;
        this->source->readbe16(length);

        auto rawd = new char[length + 1];
        this->source->read(rawd, length);
        rawd[length] = '\0';
        ref->bytes = std::string(toStdUtf8((uint8_t*) rawd, length));

        omLog(logger->info, "Utf8 #" << index << ": " << ref->bytes);

        return (OMClassConstantItem) ref;
    }

    OMClassConstantItem OMClassFileParser::parseConstantInteger(int index, uint8_t id)
    {
        auto data = new OMClassConstantInteger;
        data->type = id;
        this->source->readbe32(data->data);
        return (OMClassConstantItem) data;
    }

    OMClassConstantItem OMClassFileParser::parseConstantFloat(int index, uint8_t id)
    {
        auto data = new OMClassConstantFloat;
        data->type = id;
        this->source->readbef(data->data);
        return (OMClassConstantItem) data;
    }

    OMClassConstantItem OMClassFileParser::parseConstantString(int index, uint8_t id)
    {
        auto data = new OMClassConstantString;
        data->type = id;
        this->source->readbe16(data->stringIndex);
        return (OMClassConstantItem) data;
    }

    OMClassConstantItem OMClassFileParser::parseConstantLong(int index, uint8_t id)
    {
        auto data = new OMClassConstantLong;
        data->type = id;

        uint32_t high, low;
        this->source->readbe32(high);
        this->source->readbe32(low);

        data->data = ((uint64_t) high << 32) + low;

        omLog(logger->info, "Long #" << index << ": " << data->data);
        return (OMClassConstantItem) data;
    }

    OMClassConstantItem OMClassFileParser::parseConstantDouble(int index, uint8_t id)
    {
        auto data = new OMClassConstantDouble;
        data->type = id;

        uint32_t high, low;
        this->source->readbe32(high);
        this->source->readbe32(low);

        union
        {
            uint64_t ldata;
            double ddata;
        } d;
        d.ldata = ((uint64_t) high << 32) + low;
        data->data = d.ddata;

        omLog(logger->info, "Double #" << index << ": " << data->data);
        return (OMClassConstantItem) data;
    }
}