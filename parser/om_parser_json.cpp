#include "openminecraft/parser/om_parser_json.hpp"
#include "openminecraft/io/om_io_parser.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_string.hpp"
#include <cstdio>
#include <istream>
#include <memory>
#include <vector>

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

std::vector<OMJsonToken> OMParserJson::parse()
{
    bool inString = false;
    std::vector<OMJsonToken> data;
    while (source->good())
    {
        int k = utf8Next(source);

        switch (k)
        {
        case '{': {
            data.push_back({JsonObjectStart, "{"});
            break;
        }
        case '}': {
            data.push_back({JsonObjectEnd, "}"});
            break;
        }
        case '[': {
            data.push_back({JsonArrayStart, "["});
            break;
        }
        case ']': {
            data.push_back({JsonArrayEnd, "]"});
            break;
        }
        case '"': {
            data.push_back({(inString ? JsonStringLitrEnd : JsonStringLitrStart), "\""});
            inString = !inString;
            break;
        }
        case ':': {
            data.push_back({JsonColon, ":"});
            break;
        }
        case ',': {
            data.push_back({JsonComma, ","});
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
            std::vector<int> d;
            d.push_back(k);
            data.push_back({JsonNumber, uniToString(d)});
            break;
        }
        default: {
#define CheckChar(c) source->peek() == c
#define For(o) for (int i = 0; i < o; i++)
            if (inString)
            {
                std::vector<int> d;
                d.push_back(k);
                data.push_back({JsonString, uniToString(d)});
            }
            else if (k == 't' && CheckChar('r') && CheckChar('u') && CheckChar('e'))
            {
                data.push_back({JsonBool, "true"});
                For(3) k += source->get();
            }
            else if (k == 'f' && CheckChar('a') && CheckChar('l') && CheckChar('s') && CheckChar('e'))
            {
                data.push_back({JsonBool, "false"});
                For(4) k += source->get();
            }
            else if (k == 'n' && CheckChar('u') && CheckChar('l') && CheckChar('l'))
            {
                data.push_back({JsonBool, "null"});
                For(3) k += source->get();
            }
        }
        }
    }

    std::vector<OMJsonToken> merged;
    for (int i = 0; i < data.size(); i++)
    {
        if (i == data.size() - 1)
        {
            merged.push_back(data[i]);
        }
        else if (data[i].type == JsonNumber)
        {
            std::string r = "";
            for (int j = i; j < data.size(); j++)
            {
                if (data[j].type != JsonNumber)
                {
                    i = j - 1;
                    break;
                }
                r += (data[j].token);
            }
            merged.push_back({JsonNumber, r});
        }
        else if (data[i].type == JsonString)
        {
            std::string r = "";
            for (int j = i; j < data.size(); j++)
            {
                if (data[j].type != JsonString)
                {
                    i = j - 1;
                    break;
                }
                r += (data[j].token);
            }
            merged.push_back({JsonString, r});
        }
        else
        {
            merged.push_back(data[i]);
        }
    }

    for (auto a : merged)
    {
        logger->info("{}", a.token);
    }

    return data;
}
} // namespace openminecraft::parser::json