#ifndef OM_COMPILER_TOKEN_GSL_HPP
#define OM_COMPILER_TOKEN_GSL_HPP

#include "openminecraft/compiler/om_compiler_token.hpp"
namespace openminecraft::compiler::gsl
{
enum OMGslTokenType
{
    Qualifier,
    Type,
    Comma,
    Semicolon,
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Separator,
    NumberLiteral,
    StringLiteral,
    ConstantLiteral,
    Operator,
    Dot,
    QuestionMark,
    Colon,
};

class OMGslToken : public OMToken<OMGslTokenType>
{
  public:
    OMGslToken(OMGslTokenType type, std::string content) : OMToken<OMGslTokenType>(type, content)
    {
    }
};
} // namespace openminecraft::compiler::gsl

#endif