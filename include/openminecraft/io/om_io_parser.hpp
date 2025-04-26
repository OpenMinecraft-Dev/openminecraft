#ifndef OM_IO_PARSER_HPP
#define OM_IO_PARSER_HPP

#include <iostream>
#include <memory>

namespace openminecraft::io
{
class OMParser
{
  public:
    OMParser(std::shared_ptr<std::istream> stream);
    ~OMParser();
    bool check();

  protected:
    std::shared_ptr<std::istream> source;
};
} // namespace openminecraft::io

#endif