#ifndef OM_PARSER_JSON_HPP
#define OM_PARSER_JSON_HPP

#include "openminecraft/io/om_io_parser.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <istream>
#include <memory>

namespace openminecraft::parser::json
{
class OMParserJson : io::OMParser
{
  public:
    OMParserJson(std::shared_ptr<std::istream> s);
    ~OMParserJson();
    void parseMap();

  private:
    std::shared_ptr<log::OMLogger> logger;
};
} // namespace openminecraft::parser::json

#endif