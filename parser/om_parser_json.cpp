#include "openminecraft/parser/om_parser_json.hpp"
#include "openminecraft/io/om_io_parser.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_string.hpp"
#include <istream>
#include <memory>

using namespace openminecraft::util::string;
namespace openminecraft::parser
{
OMParserJson::OMParserJson(std::shared_ptr<std::istream> str) : io::OMParser(str)
{
    this->logger = std::make_shared<log::OMLogger>("OMParserJson", this);
}
OMParserJson::~OMParserJson()
{
    io::OMParser::~OMParser();
}

void OMParserJson::test()
{
    logger->info("{}", utf8Next(source));
}
} // namespace openminecraft::parser