#include "openminecraft/parser/om_parser_json.hpp"
#include "openminecraft/io/om_io_parser.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <istream>
#include <memory>

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
    logger->info("{}", (char)source->get());
}
} // namespace openminecraft::parser