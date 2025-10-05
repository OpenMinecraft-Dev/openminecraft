#ifndef OM_RENDERER_LAYER_VK_SWAPCHAINMANAGER_HPP
#define OM_RENDERER_LAYER_VK_SWAPCHAINMANAGER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_reinitable.hpp"
#include <functional>
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"
#include <memory>

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
    OMSwapchainManager(::vk::SurfaceKHR surface, std::function<OMSwapchainCap()> fetch, std::pair<uint32_t, uint32_t> families, ::vk::Device dev, ::vk::AllocationCallbacks callbacks, void *window);
    static ::vk::Format chooseImageFormat(OMSwapchainCap cap);
    static ::vk::ColorSpaceKHR chooseColorSpace(OMSwapchainCap cap);
    ::vk::Extent2D chooseExtent(OMSwapchainCap cap);
    static ::vk::PresentModeKHR choosePresentMode(OMSwapchainCap cap);
    ~OMSwapchainManager() = default;

    void reinit() override;
    void destroy();

private:
    std::pair<uint32_t, uint32_t> families;
    std::function<OMSwapchainCap()> fetch;
    ::vk::SurfaceKHR surface;
    ::vk::SwapchainKHR swapchain;
    ::vk::Device device;
    ::vk::AllocationCallbacks callbacks;
    void *window;
};
}

#endif