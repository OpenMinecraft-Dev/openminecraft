#ifndef OM_RENDERER_LAYER_VK_VALIDATION_HPP
#define OM_RENDERER_LAYER_VK_VALIDATION_HPP

#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include <vector>
namespace openminecraft::renderer::vk::validation
{
class OMRendererVkValidation
{
  public:
    OMRendererVkValidation(OMRendererVk renderer, std::vector<::vk::LayerProperties> props);
    ~OMRendererVkValidation();
};
} // namespace openminecraft::renderer::vk::validation

#endif