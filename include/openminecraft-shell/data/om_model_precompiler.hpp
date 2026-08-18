#ifndef OM_MODEL_PRECOMPILER_HPP
#define OM_MODEL_PRECOMPILER_HPP

#include "openminecraft/io/json/om_io_ast_json.hpp"
#include <memory>
#include <string>
namespace openminecraftshell::data
{
class OMModelPrecompiler
{
  public:
    OMModelPrecompiler(std::string root);
    ~OMModelPrecompiler() = default;

    auto precompile(std::string) -> std::shared_ptr<openminecraft::io::json::OMJsonNode>;
};
} // namespace openminecraftshell::data

#endif