#ifndef OM_COMPILER_TOKEN_JSON_HPP
#define OM_COMPILER_TOKEN_JSON_HPP

#include "openminecraft/compiler/om_compiler_token.hpp"
namespace openminecraft::compiler::json
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
} // namespace openminecraft::compiler::json

#endif
