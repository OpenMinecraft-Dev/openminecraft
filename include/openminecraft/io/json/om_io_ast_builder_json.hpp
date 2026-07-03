#ifndef OM_IO_AST_BUILDER_JSON_HPP
#define OM_IO_AST_BUILDER_JSON_HPP

#include "openminecraft/io/json/om_io_ast_json.hpp"
#include "openminecraft/io/json/om_io_tokeniter_json.hpp"
#include <memory>
#include <string>
namespace openminecraft::io::json
{
struct OMJsonAstContext
{
    std::shared_ptr<OMJsonNode> container;
    std::string key;
    bool waiting_for_value = false;

    OMJsonAstContext(std::shared_ptr<OMJsonNode> container) : container(container)
    {
    }
};

class OMJsonAstBuilder
{
  public:
    OMJsonAstBuilder(std::shared_ptr<OMJsonTokenIter> iter) : iter(iter)
    {
    }
    ~OMJsonAstBuilder() = default;

    std::shared_ptr<OMJsonNode> build();

  private:
    std::shared_ptr<OMJsonTokenIter> iter;
};
} // namespace openminecraft::io::json

#endif
