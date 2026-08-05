#ifndef OM_RENDERER_LAYER_VK_SWAPCHAINMANAGER_HPP
#define OM_RENDERER_LAYER_VK_SWAPCHAINMANAGER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_reinitable.hpp"
#include <functional>
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"

namespace openminecraft::renderer::vk::swapchain
{

struct OMSwapchainCap
{
    ::vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<::vk::SurfaceFormatKHR> formats;
    std::vector<::vk::PresentModeKHR> presentModes;
};

class OMSwapchainManager : public util::OMReinitable
{
  public:
    OMSwapchainManager(::vk::SurfaceKHR surface, std::function<OMSwapchainCap()> fetch,
                       std::pair<uint32_t, uint32_t> families, ::vk::Device dev, ::vk::AllocationCallbacks callbacks,
                       void *window);
    static auto chooseSurfaceFormat(OMSwapchainCap cap) -> ::vk::SurfaceFormatKHR;
    auto chooseExtent(const OMSwapchainCap &cap) -> ::vk::Extent2D;
    static auto choosePresentMode(const OMSwapchainCap &cap) -> ::vk::PresentModeKHR;
    ~OMSwapchainManager() = default;

    void reinit() override;
    void destroy();
    ::vk::SurfaceKHR surface;

    ::vk::SurfaceFormatKHR format;
    ::vk::Extent2D extent;
    std::vector<::vk::ImageView> swapchainImageViews;
    ::vk::SwapchainKHR swapchain;

  private:
    std::pair<uint32_t, uint32_t> families;
    std::function<OMSwapchainCap()> fetch;
    ::vk::Device device;
    ::vk::AllocationCallbacks callbacks;
    void *window;

    uint32_t swapchainImageCount;
    std::vector<::vk::Image> swapchainImages;
};
} // namespace openminecraft::renderer::vk::swapchain

#endif
