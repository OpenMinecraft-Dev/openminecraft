#include "openminecraft/renderer/vk/om_renderer_layer_vk_validation.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include <memory>

using namespace vk;
namespace openminecraft::renderer::vk::validation
{
OMRendererVkValidation::OMRendererVkValidation(OMRendererVk renderer, std::vector<LayerProperties> props)
{
    logger = std::make_shared<log::OMLogger>("OMRendererVkValidation", this);
    for (auto p : props)
    {
        if (std::string(p.layerName.data()) == "VK_LAYER_KHRONOS_validation")
        {
        }
    }
}
OMRendererVkValidation::~OMRendererVkValidation()
{
}
} // namespace openminecraft::renderer::vk::validation