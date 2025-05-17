#ifndef OM_RENDERER_LAYER_VK_VALIDATION_HPP
#define OM_RENDERER_LAYER_VK_VALIDATION_HPP

#include "openminecraft/log/om_log_common.hpp"
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"
#include <memory>
namespace openminecraft::renderer::vk::validation
{
class OMRendererVkValidation
{
  public:
    OMRendererVkValidation(std::vector<::vk::LayerProperties> props);
    ~OMRendererVkValidation();

    void attachInstance(::vk::InstanceCreateInfo i);
    ::vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    void attach(std::vector<const char *> *data);
    void attachExts(std::vector<const char *> *data);
    bool enabled = false;

  private:
    std::shared_ptr<log::OMLogger> logger;
};
} // namespace openminecraft::renderer::vk::validation

#endif