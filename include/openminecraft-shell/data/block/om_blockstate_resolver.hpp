#ifndef OM_BLOCKSTATE_RESOLVER_HPP
#define OM_BLOCKSTATE_RESOLVER_HPP

#include "openminecraft-shell/data/block/om_block.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft-shell/data/om_model_precompiler.hpp"
#include "openminecraft/log/om_log_common.hpp"
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

    void resolve(OMBlock &);

  private:
    std::string root;
    OMModelPrecompiler &compiler;
    log::OMLogger logger;
};
} // namespace openminecraftshell::data::block

#endif