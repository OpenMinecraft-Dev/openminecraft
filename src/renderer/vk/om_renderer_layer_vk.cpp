#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <memory>

namespace openminecraft::renderer::vk
{
OMRendererVk::OMRendererVk(AppInfo info) : OMRenderer(info)
{
    logger = std::make_shared<log::OMLogger>("OMRendererVk", this);
}
OMRendererVk::~OMRendererVk()
{
}
std::string OMRendererVk::driver()
{
    return "";
}
} // namespace openminecraft::renderer::vk