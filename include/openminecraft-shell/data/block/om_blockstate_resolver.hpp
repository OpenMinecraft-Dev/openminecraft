#ifndef OM_BLOCKSTATE_RESOLVER_HPP
#define OM_BLOCKSTATE_RESOLVER_HPP

#include "openminecraft-shell/data/block/om_block.hpp"
#include "openminecraft-shell/data/block/om_blockstate.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft-shell/data/om_model_precompiler.hpp"
#include "openminecraft/io/json/om_io_ast_json.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace openminecraftshell::data::block
{
class OMBlockstateResolver
{
  public:
    OMBlockstateResolver(std::string root, OMModelPrecompiler &compiler)
        : root(root), compiler(compiler), logger("OMBlockstateResolver", this)
    {
    }
    ~OMBlockstateResolver() = default;

    void resolve(OMIdentifier);
    void buildModel(OMIdentifier, OMBlockState state);
    auto fetchModel(OMIdentifier, OMBlockState state, int = 0) -> int;

  private:
    void resolveModel(OMIdentifier, std::shared_ptr<io::json::OMJsonNode>);
    auto identFrom(std::shared_ptr<io::json::OMJsonNode>) -> OMBlockModelIdentifier;

    std::unordered_map<OMIdentifier, std::unordered_map<OMBlockState, std::vector<int>>> states;
    std::unordered_map<OMIdentifier, std::shared_ptr<io::json::OMJsonNode>> resolverCache;
    std::unordered_map<OMIdentifier, std::unordered_map<OMBlockModelIdentifier, int>> requiredModels;

    std::string root;
    OMModelPrecompiler &compiler;
    log::OMLogger logger;
};
} // namespace openminecraftshell::data::block

#endif