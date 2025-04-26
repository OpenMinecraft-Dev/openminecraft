#ifndef OM_IO_PARSER_HPP
#define OM_IO_PARSER_HPP

#include <iostream>

namespace openminecraft::io {
class OMParser {
public:
    OMParser(std::istream& stream);
    ~OMParser();

protected:
    std::istream* source;
};
}

#endif