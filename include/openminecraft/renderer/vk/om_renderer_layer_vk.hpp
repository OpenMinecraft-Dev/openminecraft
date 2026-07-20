#ifndef OM_RENDERER_LAYER_VK_HPP
#define OM_RENDERER_LAYER_VK_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_validation.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <any>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "openminecraft/renderer/vk/om_renderer_layer_vk_swapchainmanager.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"

#include <map>
#include <memory>
#define VkErrorTranslate(err, key)                                                                                     \
    translate(key, translate("openminecraft.renderer.vk.err.shell",                                                    \
                             translate(fmt::format("openminecraft.renderer.vk.err.{}", err.code().message())),         \
                             (uint32_t)err.code().value()))
namespace openminecraft::renderer::vk
{
auto vkAlloc(void *, size_t size, size_t align, VkSystemAllocationScope s) -> void *;
auto vkRealloc(void *, void *o, size_t size, size_t align, VkSystemAllocationScope s) -> void *;
void vkFree(void *, void *p);
void vkInternalAlloc(void *, size_t size, VkInternalAllocationType t, VkSystemAllocationScope s);
void vkInternalFree(void *, size_t size, VkInternalAllocationType t, VkSystemAllocationScope s);

struct FrameSync
{
    std::vector<::vk::Semaphore> pipelineSemaphores;
    ::vk::Fence inFlightFence;
};

class OMRendererVk : public OMRenderer
{
  public:
    OMRendererVk(AppInfo info, std::function<int(std::vector<std::string>)> dev, void *window);
    ~OMRendererVk() override;

    auto fetchRequiredExtensions() -> util::OMResult<std::vector<const char *>, std::string>;
    auto instanceCreation(AppInfo info, std::vector<const char *> exts) -> util::OMResult<::vk::Instance, std::string>;
    auto deviceSelection(std::function<int(std::vector<std::string>)> dev)
        -> util::OMResult<::vk::PhysicalDevice, std::string>;
    auto sdlVulkanLoading() -> util::OMResult<std::any, std::string>;
    auto deviceCreation() -> util::OMResult<::vk::Device, std::string>;
    auto deviceQueueFetch() -> util::OMResult<std::any, std::string>;
    auto getSwapchainCap() -> swapchain::OMSwapchainCap;

    auto driver() -> std::string override;
    auto allocateBuffer(common::OMBufferUsage usage, uint64_t length) -> common::OMRendererBuffer * override;
    auto allocateTexture(uint64_t width, uint64_t height, common::OMTextureType type, common::OMTextureArrangement arr)
        -> common::OMRendererTexture * override;
    auto createRenderTarget() -> common::OMRendererRenderTarget * override;
    auto getDefaultRenderTarget() -> common::OMRendererRenderTarget * override;
    auto createPipeline() -> common::OMRendererPipeline * override;
    auto createTask() -> common::OMRendererTask * override;
    auto getExtent() const -> glm::vec2 override;

    void baseInit() override;

    void render() override;
    void requestResize() override;

    std::shared_ptr<validation::OMRendererVkValidation> validationLayer;
    std::shared_ptr<swapchain::OMSwapchainManager> swapchainManager;

    common::OMRendererRenderTarget *defaultTarget;
    common::OMRendererTexture *defaultDepthBuffer;
    std::vector<::vk::Framebuffer> defaultFramebuffers;
    std::vector<::vk::CommandBuffer> defaultCommandBuffers;

    ::vk::AllocationCallbacks allocator;
    ::vk::Instance instance;
    ::vk::PhysicalDevice physicalDevice;
    ::vk::Device logicalDevice;
    ::vk::DebugReportCallbackEXT reportCallback;
    ::vk::SurfaceKHR surface;
    std::pair<uint32_t, uint32_t> queueFamilyIndex;
    std::pair<::vk::Queue, ::vk::Queue> queues;

    ::vk::CommandPool tempCommandPool;

    std::vector<FrameSync> frameSyncs;
    std::vector<::vk::Semaphore> frameRenderSemaphores;
    std::map<uint32_t, FrameSync> inflights;

    int framesInFlight = 3;
    int thisFrame = 0;

    bool needRebuild = false;

  private:
    void rebuildDefaults();

    std::shared_ptr<log::OMLogger> logger;
    ::vk::PhysicalDeviceMemoryProperties memProps;
};
}; // namespace openminecraft::renderer::vk

#endif
