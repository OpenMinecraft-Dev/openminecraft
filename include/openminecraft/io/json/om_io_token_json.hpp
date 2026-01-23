#ifndef OM_IO_TOKEN_JSON_HPP
#define OM_IO_TOKEN_JSON_HPP

#include "openminecraft/io/om_io_token.hpp"
namespace openminecraft::io::json
{
enum OMJsonTokenType
{
    BeginObject,
    BeginArray,
    NumberLiteral,
    StringLiteral,
    ConstantLiteral,
    Comma,
    Colon,
    EndObject,
    EndArray
};

class OMJsonToken : public OMToken<OMJsonTokenType>
{
  public:
    OMJsonToken(OMJsonTokenType type, std::string content) : OMToken<OMJsonTokenType>(type, content)
    {
    }
};
} // namespace openminecraft::io::json

#endif
