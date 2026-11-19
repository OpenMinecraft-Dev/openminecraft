#ifndef OM_COMPILER_TOKENITER_JSON_HPP
#define OM_COMPILER_TOKENITER_JSON_HPP

#include "openminecraft/compiler/json/om_compiler_token_json.hpp"
#include "openminecraft/compiler/om_compiler_tokeniter.hpp"
#include <memory>
namespace openminecraft::compiler::json
{
class OMJsonTokenIter : public OMTokenIter<OMJsonToken>
{
  public:
    OMJsonTokenIter(std::shared_ptr<std::istream> istr) : OMTokenIter(istr)
    {
    }
    virtual ~OMJsonTokenIter() = default;

    auto next() -> std::shared_ptr<OMJsonToken> override;
    auto end() -> bool override;
};
} // namespace openminecraft::compiler::json

#endif
