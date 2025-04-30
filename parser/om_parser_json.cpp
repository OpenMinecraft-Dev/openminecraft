#include "openminecraft/parser/om_parser_json.hpp"
#include "openminecraft/io/om_io_parser.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_string.hpp"
#include <istream>
#include <memory>

using namespace openminecraft::util::string;
namespace openminecraft::parser::json
{
OMParserJson::OMParserJson(std::shared_ptr<std::istream> str) : io::OMParser(str)
{
    this->logger = std::make_shared<log::OMLogger>("OMParserJson", this);
}
OMParserJson::~OMParserJson()
{
    io::OMParser::~OMParser();
}

void OMParserJson::parseMap()
{
    bool inString = false;
    while (source->good())
    {
        int k = utf8Next(source);

        switch (k)
        {
        case '{': {
            logger->info("JsonObjectStart");
            break;
        }
        case '}': {
            logger->info("JsonObjectEnd");
            break;
        }
        case '[': {
            logger->info("JsonArrayStart");
            break;
        }
        case ']': {
            logger->info("JsonArrayEnd");
            break;
        }
        case '"': {
            logger->info("JsonStringLitr{}", inString ? "End" : "Start");
            inString = !inString;
            break;
        }
        case ':': {
            logger->info("JsonColon");
            break;
        }
        case ',': {
            logger->info("JsonComma");
            break;
        }
        case '.':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            logger->info("JsonNumber");
            break;
        }
        default: {
            if (inString)
            {
                logger->info("JsonString");
            }
        }
        }
    }
}
} // namespace openminecraft::parser::json