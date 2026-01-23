#ifndef OM_IO_TOKENITER_JSON
#define OM_IO_TOKENITER_JSON

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
    ~OMJsonTokenIter() = default;

    std::shared_ptr<OMJsonToken> next() override;
    bool end() override;
};
} // namespace openminecraft::io::json

#endif
