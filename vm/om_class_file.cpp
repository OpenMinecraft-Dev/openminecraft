#include "openminecraft/log/om_log_common.hpp"
#include <exception>
#include <fstream>
#include <openminecraft/vm/om_class_file.hpp>
#include <openminecraft/binary/om_bin_endians.hpp>
#include <stdexcept>
#include <vector>

using namespace openminecraft::binary;

namespace openminecraft::vm::classfile
{
    OMClassFileParser::OMClassFileParser(std::ifstream& str)
    {
        this->source = &str;
        this->logger = new log::OMLogger("OMClassFileParser");
    }

    OMClassFileParser::~OMClassFileParser()
    {
        delete this->source;
        delete this->logger;
    }

    OMClassFile* OMClassFileParser::parse()
    {
        auto file = new OMClassFile;
        this->source->read((char*) &file->magicNumber, sizeof(uint32_t));
        be32(file->magicNumber);
        this->source->read((char*) &file->major, sizeof(uint16_t));
        be16(file->minor);
        this->source->read((char*) &file->major, sizeof(uint16_t));
        be16(file->major);
        this->source->read((char*) &file->constantPoolCount, sizeof(uint16_t));
        be16(file->constantPoolCount);
        file->constants = std::vector<OMClassConstantItem>(file->constantPoolCount);

        for (int i = 0; i < file->constantPoolCount; i++)
        {
            file->constants[i] = parseConstant(i);
        }

        return file;
    }

    OMClassConstantItem OMClassFileParser::parseConstant(int index)
    {
        uint8_t id;
        this->source->read((char*) &id, 1);
        switch (id)
        {
            case JVM_Constant_Class:
                return parseConstantClass(index, id);
            case JVM_Constant_FieldRef:
            case JVM_Constant_MethodRef:
            case JVM_Constant_InterfaceMethodRef:
                return parseConstantRef(index, id);
        }

        throw std::invalid_argument("Unknown constant pool id!");
    }

    OMClassConstantItem OMClassFileParser::parseConstantClass(int index, uint8_t id)
    {
        auto cls = new OMClassConstantClass;
        cls->type = id;
        this->source->read((char*) &cls->nameIndex, sizeof(uint16_t));
        be16(cls->nameIndex);
        omLog(logger->info, "Class 0x" << std::hex << (int) cls->type << " #" << index << ": #" << cls->nameIndex);
        return (OMClassConstantItem) cls;
    }

    OMClassConstantItem OMClassFileParser::parseConstantRef(int index, uint8_t id)
    {
        auto ref = new OMClassConstantRef;
        ref->type = id;
        this->source->read((char*) &ref->classIndex, sizeof(uint16_t));
        be16(ref->classIndex);
        this->source->read((char*) &ref->nameAndTypeIndex, sizeof(uint16_t));
        be16(ref->nameAndTypeIndex);
        omLog(logger->info, "Ref 0x" << std::hex << (int) ref->type << " #" << index << ": #" << ref->classIndex << ".#" << ref->nameAndTypeIndex);
        return (OMClassConstantItem) ref;
    }
}