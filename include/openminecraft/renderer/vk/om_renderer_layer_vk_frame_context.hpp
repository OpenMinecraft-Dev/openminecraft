#ifndef OM_RENDERER_LAYER_VK_FRAME_CONTEXT_HPP
#define OM_RENDERER_LAYER_VK_FRAME_CONTEXT_HPP

#include "openminecraft/renderer/common/om_renderer_frame_context.hpp"
#include "openminecraft/renderer/common/om_renderer_sync_point.hpp"
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"
#include <vector>

namespace openminecraft::renderer::vk
{
class OMRendererVk;
class OMRendererGpuEventVk;
class OMRendererGpuCpuBridgeVk;

class OMRendererFrameContextVk : public common::OMRendererFrameContext
{
  public:
    OMRendererFrameContextVk(OMRendererVk *renderer, uint32_t index);
    ~OMRendererFrameContextVk() override;

    auto index() const -> uint32_t override
    {
        return frameIndex;
    }

    void begin() override;
    void end() override;

    // Performs the frame-slot synchronization and acquires the next swapchain
    // image. Returns false when the swapchain must be rebuilt.
    auto beginFrame() -> bool;
    auto submit(const std::vector<::vk::CommandBuffer> &commands, uint32_t layer, bool lastLayer) -> void;
    auto present() -> bool;

    auto imageIndex() const -> uint32_t
    {
        return currentImage;
    }

    auto nextFrameSerial() const -> uint64_t
    {
        return frameSerial + 1;
    }

    auto currentFrameSerial() const -> uint64_t
    {
        return frameSerial;
    }

    auto isFrameComplete(uint64_t serial) -> bool;

    auto inFlightFence() const -> common::OMRendererGpuCpuBridge *
    {
        return syncFence;
    }

    void ensurePipelineSemaphores(size_t count);

  private:
    auto imageAvailable() -> common::OMRendererGpuEvent *;

    OMRendererVk *renderer;
    uint32_t frameIndex;
    uint32_t currentImage = 0;
    uint64_t frameSerial = 0;

    std::vector<common::OMRendererGpuEvent *> pipelineSemaphores;
    common::OMRendererGpuCpuBridge *syncFence = nullptr;
    common::OMRendererGpuEvent *renderFinished = nullptr;
};
} // namespace openminecraft::renderer::vk

#endif
