#include <fstream>
#include <openminecraft/vm/om_class_file.hpp>

namespace openminecraft::vm::classfile
{
    OMClassFileParser::OMClassFileParser(std::ifstream& str)
    {
        this->source = &str;
    }

    OMClassFileParser::~OMClassFileParser()
    {
        delete this->source;
    }

    OMClassFile* OMClassFileParser::parse()
    {
        auto file = new OMClassFile;

        return file;
    }
}