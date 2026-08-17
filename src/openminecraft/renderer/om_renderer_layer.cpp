#include <utility>

#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/common/om_renderer_shadermanager.hpp"

namespace openminecraft::renderer
{
OMRenderer::OMRenderer(AppInfo info, void *window, std::string shaderPath)
    : info(std::move(info)), window(window), shaderManager(shaderPath)
{
}
} // namespace openminecraft::renderer
