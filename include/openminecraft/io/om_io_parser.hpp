#ifndef OM_IO_PARSER_HPP
#define OM_IO_PARSER_HPP

#include <iostream>

namespace openminecraft::io
{
class OMParser
{
  public:
    OMParser(std::istream &stream);
    ~OMParser();
    bool check();

  protected:
    std::istream *source;
};
} // namespace openminecraft::io

#endif