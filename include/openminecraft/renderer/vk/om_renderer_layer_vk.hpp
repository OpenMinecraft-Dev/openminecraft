#ifndef OM_RENDERER_LAYER_VK_HPP
#define OM_RENDERER_LAYER_VK_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
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
    OMRendererVk(AppInfo info);
    ~OMRendererVk();

    virtual std::string driver() override;

  private:
    std::shared_ptr<log::OMLogger> logger;
    ::vk::Instance instance;
};
}; // namespace openminecraft::renderer::vk

#endif