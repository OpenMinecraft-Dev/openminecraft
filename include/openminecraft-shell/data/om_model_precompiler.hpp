#ifndef OM_MODEL_PRECOMPILER_HPP
#define OM_MODEL_PRECOMPILER_HPP

#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/io/json/om_io_ast_json.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <memory>
#include <string>
namespace openminecraftshell::data
{
class OMModelPrecompiler
{
  public:
    OMModelPrecompiler(std::string root);
    ~OMModelPrecompiler() = default;

    auto precompile(OMIdentifier, bool = true) -> std::shared_ptr<openminecraft::io::json::OMJsonNode>;

  private:
    std::string root;
    openminecraft::log::OMLogger logger;
};
} // namespace openminecraftshell::data

#endif