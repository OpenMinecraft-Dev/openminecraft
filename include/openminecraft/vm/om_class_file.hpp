#ifndef OM_CLASS_FILE_HPP
#define OM_CLASS_FILE_HPP

#include <cstdint>
#include <fstream>

namespace openminecraft::vm::classfile
{
    struct OMClassFile
    {
        uint32_t magicNumber;
        uint16_t minor;
        uint16_t major;
    };

    class OMClassFileParser
    {
        public:
            OMClassFileParser(std::ifstream& stream);
            ~OMClassFileParser();
            OMClassFile* parse();

        private:
            std::ifstream* source;
    };
}

#endif
