#ifndef OM_RENDERER_LAYER_VK_HPP
#define OM_RENDERER_LAYER_VK_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_validation.hpp"
#include <functional>
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include <memory>
namespace openminecraft::renderer::vk
{
void *vkAlloc(void *, size_t size, size_t align, VkSystemAllocationScope s);
void *vkRealloc(void *, void *o, size_t size, size_t align, VkSystemAllocationScope s);
void vkFree(void *, void *p);
void vkInternalAlloc(void *, size_t size, VkInternalAllocationType t, VkSystemAllocationScope s);
void vkInternalFree(void *, size_t size, VkInternalAllocationType t, VkSystemAllocationScope s);
class OMRendererVk : public OMRenderer
{
  public:
    OMRendererVk(AppInfo info, std::function<int(std::vector<std::string>)> dev);
    ~OMRendererVk();

    virtual std::string driver() override;

    std::shared_ptr<validation::OMRendererVkValidation> validationLayer;
    ::vk::AllocationCallbacks allocator;
    ::vk::Instance instance;
    ::vk::PhysicalDevice physicalDevice;

  private:
    std::shared_ptr<log::OMLogger> logger;
#ifdef OM_VULKAN_DYNAMIC
    ::vk::detail::DynamicLoader loader;
#endif
};
}; // namespace openminecraft::renderer::vk

#endif