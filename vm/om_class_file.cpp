#include "openminecraft/log/om_log_common.hpp"
#include <cstdint>
#include <openminecraft/vm/om_class_file.hpp>
#include <openminecraft/binary/om_bin_endians.hpp>
#include <stdexcept>
#include <vector>

using namespace openminecraft::binary;

namespace openminecraft::vm::classfile
{
    // Constants
    OMClassConstantMethodRef::OMClassConstantMethodRef(uint16_t ci, uint16_t nti): classIndex(ci), nameAndTypeIndex(nti) {}
    OMClassConstantType OMClassConstantMethodRef::type() { return OMClassConstantType::MethodRef; }

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
        file->constants = std::vector<OMClassConstant*>(file->constantPoolCount - 1);

        uint16_t idx = 0;
        while (idx < file->constantPoolCount - 1)
        {
            file->constants.push_back(this->parseConstant(&idx));
        }

        return file;
    }

    OMClassConstant* OMClassFileParser::parseConstant(uint16_t* idx)
    {
        (*idx)++;

        OMClassConstantType type;
        this->source->read((char*) &type, 1);

        uint16_t temp1, temp2, temp3, temp4;

        OMClassConstant* result = nullptr;
        switch (type) {
            case OMClassConstantType::MethodRef:
            {
                this->source->readbe16(temp1);
                this->source->readbe16(temp2);
                result = new OMClassConstantMethodRef(temp1, temp2);
                omLog(this->logger->info, "#" << *idx << " MethodRef(#" << temp1 << ", #" << temp2 << ")");
                break;
            }
            default:
                throw std::invalid_argument("Unknown constant type id!");
                break;
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