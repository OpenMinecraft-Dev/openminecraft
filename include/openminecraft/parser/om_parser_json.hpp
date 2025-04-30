#ifndef OM_PARSER_JSON_HPP
#define OM_PARSER_JSON_HPP

#include "openminecraft/io/om_io_parser.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <istream>
#include <memory>
#include <vector>

namespace openminecraft::parser::json
{
enum OMJsonTokens
{
    JsonStart,
    JsonObject,
    JsonArray,
    JsonKey,
    JsonColon,
    JsonString,
    JsonNumber,
    JsonLiteral,
    JsonComma,
    JsonEscape
};

class OMParserJson : io::OMParser
{
  public:
    OMParserJson(std::shared_ptr<std::istream> s);
    ~OMParserJson();
    void test();

  private:
    OMJsonTokens token = JsonStart;
    std::vector<int> keytemp;
    std::shared_ptr<log::OMLogger> logger;
};
} // namespace openminecraft::parser::json

#endif