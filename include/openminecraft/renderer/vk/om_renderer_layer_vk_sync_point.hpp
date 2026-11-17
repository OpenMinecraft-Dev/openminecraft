#ifndef OM_RENDERER_LAYER_VK_SYNC_POINT_HPP
#define OM_RENDERER_LAYER_VK_SYNC_POINT_HPP

#include "openminecraft/renderer/common/om_renderer_sync_point.hpp"
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"

namespace openminecraft::renderer::vk
{
class OMRendererGpuEventVk : public common::OMRendererGpuEvent
{
  public:
    OMRendererGpuEventVk(::vk::Device device, ::vk::AllocationCallbacks allocator);
    ~OMRendererGpuEventVk() override;

    auto semaphore() const -> ::vk::Semaphore
    {
        return handle;
    }

  private:
    ::vk::Device device;
    ::vk::AllocationCallbacks allocator;
    ::vk::Semaphore handle;
};

class OMRendererGpuCpuBridgeVk : public common::OMRendererGpuCpuBridge
{
  public:
    OMRendererGpuCpuBridgeVk(::vk::Device device, ::vk::AllocationCallbacks allocator, bool signaled);
    ~OMRendererGpuCpuBridgeVk() override;

    void waitCpu() override;
    auto isSignaled() -> bool override;
    void reset() override;

    auto fence() const -> ::vk::Fence
    {
        return handle;
    }

  private:
    ::vk::Device device;
    ::vk::AllocationCallbacks allocator;
    ::vk::Fence handle;
};
} // namespace openminecraft::renderer::vk

#endif
