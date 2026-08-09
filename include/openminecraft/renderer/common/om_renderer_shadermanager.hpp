#ifndef OM_RENDERER_SHADERMANAGER_HPP
#define OM_RENDERER_SHADERMANAGER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include <memory>
#include <string>
#include <utility>

namespace openminecraft::renderer::common
{
class OMRendererShaderManager
{
  public:
    OMRendererShaderManager(std::string root) : root(std::move(root)), logger("OMRendererShaderManager", this)
    {
    }
    ~OMRendererShaderManager() = default;

    auto preprocess(std::string name, OMShaderType, OMShaderFileType, basics::OMVertexFormat &)
        -> std::shared_ptr<OMShader>;

  private:
    std::string root;
    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common

#endif
