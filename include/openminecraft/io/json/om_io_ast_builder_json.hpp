#ifndef OM_IO_AST_BUILDER_JSON_HPP
#define OM_IO_AST_BUILDER_JSON_HPP

#include "openminecraft/io/json/om_io_ast_json.hpp"
#include "openminecraft/io/json/om_io_tokeniter_json.hpp"
#include <memory>
#include <string>
#include <utility>
namespace openminecraft::io::json
{
struct OMJsonAstContext
{
    std::shared_ptr<OMJsonNode> container;
    std::string key;
    bool waiting_for_value = false;

    OMJsonAstContext(std::shared_ptr<OMJsonNode> container) : container(std::move(container))
    {
    }
};

class OMJsonAstBuilder
{
  public:
    OMJsonAstBuilder(std::shared_ptr<OMJsonTokenIter> iter) : iter(std::move(iter))
    {
    }
    ~OMJsonAstBuilder() = default;

    auto build() -> std::shared_ptr<OMJsonNode>;

  private:
    std::shared_ptr<OMJsonTokenIter> iter;
};
} // namespace openminecraft::io::json

#endif
