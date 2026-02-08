#ifndef OM_RENDERER_LAYER_VK_HPP
#define OM_RENDERER_LAYER_VK_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_validation.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <any>
#include <functional>
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "om_renderer_layer_vk_swapchainmanager.hpp"
#include "om_renderer_layer_vk_testrenderer.hpp"
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
void *vkAlloc(void *, size_t size, size_t align, VkSystemAllocationScope s);
void *vkRealloc(void *, void *o, size_t size, size_t align, VkSystemAllocationScope s);
void vkFree(void *, void *p);
void vkInternalAlloc(void *, size_t size, VkInternalAllocationType t, VkSystemAllocationScope s);
void vkInternalFree(void *, size_t size, VkInternalAllocationType t, VkSystemAllocationScope s);

struct FrameSync
{
    ::vk::Semaphore imageAvailableSemaphore;
    ::vk::Fence inFlightFence;
};

class OMRendererVk : public OMRenderer
{
  public:
    OMRendererVk(AppInfo info, std::function<int(std::vector<std::string>)> dev, void *window);
    ~OMRendererVk();

    util::OMResult<std::vector<const char *>, std::string> fetchRequiredExtensions();
    util::OMResult<::vk::Instance, std::string> instanceCreation(AppInfo info, std::vector<const char *> exts);
    util::OMResult<::vk::PhysicalDevice, std::string> deviceSelection(std::function<int(std::vector<std::string>)> dev);
    util::OMResult<std::any, std::string> sdlVulkanLoading();
    util::OMResult<::vk::Device, std::string> deviceCreation();
    util::OMResult<std::any, std::string> deviceQueueFetch();
    swapchain::OMSwapchainCap getSwapchainCap();

    std::string driver() override;
    common::OMRendererBuffer *allocateBuffer(common::OMBufferUsage usage, uint64_t length) override;
    common::OMRendererTexture *allocateTexture(uint64_t width, uint64_t height, common::OMTextureType type,
                                               common::OMTextureArrangement arr) override;
    common::OMRendererRenderTarget *createRenderTarget() override;
    common::OMRendererRenderTarget *getDefaultRenderTarget() override;
    common::OMRendererPipeline *createPipeline() override;
    common::OMRendererTask *createTask() override;
    void attachTask(common::OMRendererTask *task) override;
    glm::vec2 getExtent() const override;

    void render();

    std::shared_ptr<validation::OMRendererVkValidation> validationLayer;
    std::shared_ptr<swapchain::OMSwapchainManager> swapchainManager;
    test::OMTestRenderer *testRenderer;

    common::OMRendererRenderTarget *defaultTarget;
    common::OMRendererTexture *defaultDepthBuffer;
    std::vector<::vk::Framebuffer> defaultFramebuffers;
    std::vector<::vk::CommandBuffer> defaultCommandBuffers;

    std::vector<common::OMRendererTask *> tasks;

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
