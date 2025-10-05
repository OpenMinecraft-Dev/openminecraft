#include "openminecraft/renderer/vk/om_renderer_layer_vk_swapchainmanager.hpp"

#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"

#include <SDL3/SDL_video.h>

using namespace ::vk;

namespace openminecraft::renderer::vk::swapchain
{
OMSwapchainManager::OMSwapchainManager(SurfaceKHR surface, std::function<OMSwapchainCap()> fetch, std::pair<uint32_t, uint32_t> families, Device dev, AllocationCallbacks callbacks, void *window): families(families), fetch(fetch), surface(surface), device(dev), callbacks(callbacks), window(window)
{
    OMSwapchainManager::reinit();
}

Format OMSwapchainManager::chooseImageFormat(OMSwapchainCap cap)
{
    return Format::eB8G8R8Unorm;
}

ColorSpaceKHR OMSwapchainManager::chooseColorSpace(OMSwapchainCap cap)
{
    return ColorSpaceKHR::eAdobergbNonlinearEXT;
}

Extent2D OMSwapchainManager::chooseExtent(OMSwapchainCap cap)
{
    if (cap.capabilities.currentExtent.width != -1)
    {
        return cap.capabilities.currentExtent;
    }

    int w = 0, h = 0;
    SDL_GetWindowSize(static_cast<SDL_Window *>(window), &w, &h);

    return VkExtent2D{
        std::min(cap.capabilities.maxImageExtent.width, std::max(static_cast<uint32_t>(w), cap.capabilities.minImageExtent.width)),
        std::min(cap.capabilities.maxImageExtent.height, std::max(static_cast<uint32_t>(h), cap.capabilities.minImageExtent.height))
    };
}

PresentModeKHR OMSwapchainManager::choosePresentMode(OMSwapchainCap cap)
{
    for (auto pm : cap.presentModes)
    {
        if (pm == PresentModeKHR::eMailbox || pm == PresentModeKHR::eImmediate)
        {
            return pm;
        }
    }

    return PresentModeKHR::eFifo;
}

void OMSwapchainManager::reinit()
{
    auto supp = fetch();
    auto imageCount = supp.capabilities.minImageCount + 1;
    if (supp.capabilities.maxImageCount > 0 && imageCount > supp.capabilities.maxImageCount)
    {
        imageCount = supp.capabilities.maxImageCount;
    }

    SwapchainCreateInfoKHR createInfo(
        {},
        surface,
        imageCount,
        chooseImageFormat(supp),
        chooseColorSpace(supp),
        chooseExtent(supp),
        1,
        ImageUsageFlagBits::eColorAttachment,
        {},
        {},
        {},
        supp.capabilities.currentTransform,
        CompositeAlphaFlagBitsKHR::eInherit,
        choosePresentMode(supp),
        false,
        nullptr
        );

    if (families.first != families.second)
    {
        createInfo.imageSharingMode = SharingMode::eConcurrent;
        uint32_t data[] = {families.first, families.second};
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = data;
    }
    else
    {
        createInfo.imageSharingMode = SharingMode::eExclusive;
    }

    swapchain = device.createSwapchainKHR(createInfo, callbacks);

    // device.getSwapchainImagesKHR();
}

void OMSwapchainManager::destroy()
{
    device.destroySwapchainKHR(swapchain, callbacks);
}
}