#include <utility>

#include "openminecraft/renderer/om_renderer_layer.hpp"

namespace openminecraft::renderer
{
OMRenderer::OMRenderer(AppInfo info, void *window) : info(std::move(info)), window(window)
{
}
} // namespace openminecraft::renderer
