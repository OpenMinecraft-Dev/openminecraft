#include "openminecraft/io/json/om_io_tokeniter_json.hpp"
#include "openminecraft/io/json/om_io_token_json.hpp"
#include "openminecraft/io/om_io_tokeniter_exception.hpp"
#include <array>
#include <fmt/format.h>
#include <iostream>
#include <memory>
#include <cstring>

namespace openminecraft::io::json
{
auto OMJsonTokenIter::next() -> std::shared_ptr<OMJsonToken>
{
beg:
    switch (this->source->peek())
    {
    case '{':
        this->source->ignore(1);
        return std::make_shared<OMJsonToken>(BeginObject, "{");
    case '[':
        this->source->ignore(1);
        return std::make_shared<OMJsonToken>(BeginArray, "[");
    case '"': {
        this->source->ignore(1);
        std::string target = "";
        while (true)
        {
            char c;
            this->source->read(&c, 1);

            if (c == '"')
            {
                break;
            }
            if (c == '\n')
            {
                throw OMTokenIterException("json: unexpected line inside a string!");
            }

            target += c;
        }
        return std::make_shared<OMJsonToken>(StringLiteral, target);
    }
    case ':':
        this->source->ignore(1);
        return std::make_shared<OMJsonToken>(Colon, ":");
    case ',':
        this->source->ignore(1);
        return std::make_shared<OMJsonToken>(Comma, ",");
    case ' ':
    case '\n':
    case '\t':
    case '\r':
        this->source->ignore(1);
        goto beg;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '.': {
        std::string target;
        while (true)
        {
            char c = this->source->peek();

            if ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == '-')
            {
                target += c;
                this->source->ignore(1);
            }
            else
            {
                break;
            }
        }
        return std::make_shared<OMJsonToken>(NumberLiteral, target);
    }
    case 't':
    case 'n': {
        std::array<char, 5> b;
	b[4] = '\0';
        this->source->read(b.data(), 4);
        if (std::strncmp(b.data(), "true", 4) == 0 || std::strncmp(b.data(), "null", 4) == 0)
        {
            return std::make_shared<OMJsonToken>(ConstantLiteral, b.data());
        }
        else
        {
            throw OMTokenIterException(fmt::format("json: unknown bad constant {}", b.data()));
        }
    }
    case 'f': {
        std::array<char, 6> b;
	b[5] = '\0';
        this->source->read(b.data(), 5);
        if (std::strncmp(b.data(), "false", 5) == 0)
        {
            return std::make_shared<OMJsonToken>(ConstantLiteral, b.data());
        }
        else
        {
            throw OMTokenIterException(fmt::format("json: unknown bad constant {}", b.data()));
        }
    }
    case '}':
        this->source->ignore(1);
        return std::make_shared<OMJsonToken>(EndObject, "}");
    case ']':
        this->source->ignore(1);
        return std::make_shared<OMJsonToken>(EndArray, "]");
    }

    if (this->end())
    {
        return nullptr;
    }

    throw OMTokenIterException(fmt::format("json: unknown token {}", static_cast<char>(this->source->peek())));
}

auto OMJsonTokenIter::end() -> bool
{
    return !this->check();
}
} // namespace openminecraft::io::json
