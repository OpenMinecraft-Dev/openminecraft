#ifndef OM_COMPILER_AST_BUILDER_JSON_HPP
#define OM_COMPILER_AST_BUILDER_JSON_HPP

#include "openminecraft/compiler/json/om_compiler_ast_json.hpp"
#include "openminecraft/compiler/json/om_compiler_tokeniter_json.hpp"
#include <memory>
#include <string>
#include <utility>
namespace openminecraft::compiler::json
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
} // namespace openminecraft::compiler::json

#endif
