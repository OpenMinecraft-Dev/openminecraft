#ifndef OM_IO_TOKENITER_JSON_HPP
#define OM_IO_TOKENITER_JSON_HPP

#include "openminecraft/io/json/om_io_token_json.hpp"
#include "openminecraft/io/om_io_tokeniter.hpp"
#include <memory>
namespace openminecraft::io::json
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
} // namespace openminecraft::io::json

#endif
