#ifndef OM_RENDERER_LAYER_VK_VALIDATION_HPP
#define OM_RENDERER_LAYER_VK_VALIDATION_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include <memory>
#include <vector>
namespace openminecraft::renderer::vk::validation
{
class OMRendererVkValidation
{
  public:
    OMRendererVkValidation(OMRendererVk renderer, std::vector<::vk::LayerProperties> props);
    ~OMRendererVkValidation();

  private:
    std::shared_ptr<log::OMLogger> logger;
};
} // namespace openminecraft::renderer::vk::validation

#endif