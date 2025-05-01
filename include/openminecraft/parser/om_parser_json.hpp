#ifndef OM_PARSER_JSON_HPP
#define OM_PARSER_JSON_HPP

#include "openminecraft/io/om_io_parser.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <istream>
#include <memory>
#include <vector>

namespace openminecraft::parser::json
{
enum OMJsonTokenType
{
    JsonEmpty,
    JsonNull,
    JsonBool,
    JsonObjectStart,
    JsonObjectEnd,
    JsonArrayStart,
    JsonArrayEnd,
    JsonComma,
    JsonColon,
    JsonNumber,
    JsonString
};
struct OMJsonToken
{
    OMJsonTokenType type;
    std::string token;
};
class OMParserJson : io::OMParser
{
  public:
    OMParserJson(std::shared_ptr<std::istream> s);
    ~OMParserJson();
    std::vector<OMJsonToken> parse();

  private:
    std::shared_ptr<log::OMLogger> logger;
};
} // namespace openminecraft::parser::json

#endif