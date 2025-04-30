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

void OMParserJson::test()
{
    int k;
    while (source->good())
    {
        k = utf8Next(source);

        switch (token)
        {
        case JsonStart: {
            if (k != '{')
            {
                // bad json!
                continue;
            }
            else
            {
                logger->info("-> JsonObject");
                token = JsonObject;
            }
            break;
        }
        case JsonObject: {
            if (k != '"')
            {
                // bad json key!
                continue;
            }
            else
            {
                logger->info("-> JsonKey");
                token = JsonKey;
            }
            break;
        }
        case JsonKey: {
            if (k == '"')
            {
                logger->info("\"{}\"-> JsonColon", uniToString(keytemp));
                keytemp.clear();
                token = JsonColon;
            }
            keytemp.push_back(k);
            break;
        }
        default: {
            break;
        }
        }
    }
}
} // namespace openminecraft::parser::json