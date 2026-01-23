#include "openminecraft/io/json/om_io_tokeniter_json.hpp"
#include "openminecraft/io/json/om_io_token_json.hpp"
#include "openminecraft/io/om_io_tokeniter_exception.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <memory>

namespace openminecraft::io::json
{
std::shared_ptr<OMJsonToken> OMJsonTokenIter::next()
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
        return std::make_shared<OMJsonToken>(Comma, ":");
    case ' ':
    case '\n':
    case '\t':
        this->source->ignore(1);
        goto beg;
    }

    throw OMTokenIterException(fmt::format("json: unknown token {}", static_cast<char>(this->source->peek())));
}

bool OMJsonTokenIter::end()
{
    return false;
}
} // namespace openminecraft::io::json
