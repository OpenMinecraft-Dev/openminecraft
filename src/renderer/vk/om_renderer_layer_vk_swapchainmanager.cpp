#include "openminecraft/renderer/vk/om_renderer_layer_vk_swapchainmanager.hpp"

#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"

#include <SDL3/SDL_video.h>

#include <utility>

using namespace ::vk;

namespace openminecraft::renderer::vk::swapchain
{
OMSwapchainManager::OMSwapchainManager(SurfaceKHR surface, std::function<OMSwapchainCap()> fetch, std::pair<uint32_t, uint32_t> families, Device dev, AllocationCallbacks callbacks, void *window): families(std::move(families)), fetch(std::move(fetch)), surface(surface), device(dev), callbacks(callbacks), window(window)
{
    OMSwapchainManager::reinit();
}

SurfaceFormatKHR OMSwapchainManager::chooseSurfaceFormat(OMSwapchainCap cap)
{
    for (auto sf : cap.formats)
    {
        if (sf.format == Format::eB8G8R8Unorm && sf.colorSpace == ColorSpaceKHR::eSrgbNonlinear)
        {
            return sf;
        }
    }

    return cap.formats[0];
}

Extent2D OMSwapchainManager::chooseExtent(const OMSwapchainCap &cap)
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

PresentModeKHR OMSwapchainManager::choosePresentMode(const OMSwapchainCap& cap)
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

    auto form = chooseSurfaceFormat(supp);
    this->format = form;

    extent = chooseExtent(supp);

    SwapchainCreateInfoKHR createInfo(
        {},
        surface,
        imageCount,
        form.format,
        form.colorSpace,
        extent,
        1,
        ImageUsageFlagBits::eColorAttachment,
        {},
        {},
        {},
        supp.capabilities.currentTransform,
        CompositeAlphaFlagBitsKHR::eOpaque,
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

    auto target = device.getSwapchainImagesKHR(swapchain, &swapchainImageCount, nullptr);
    if (target != Result::eSuccess)
    {
        throw SystemError(target);
    }

    std::vector<Image> images(swapchainImageCount);
    target = device.getSwapchainImagesKHR(swapchain, &swapchainImageCount, images.data());
    if (target != Result::eSuccess)
    {
        throw SystemError(target);
    }

    swapchainImages = images;

    for (auto img : images)
    {
        ImageViewCreateInfo createInfo({}, img, ImageViewType::e2D, form.format, {ComponentSwizzle::eIdentity, ComponentSwizzle::eIdentity, ComponentSwizzle::eIdentity, ComponentSwizzle::eIdentity}, ImageSubresourceRange(ImageAspectFlagBits::eColor, 0, 1, 0, 1));

        ImageView imgv;
        target = device.createImageView(&createInfo, &callbacks, &imgv);
        if (target != Result::eSuccess)
        {
            throw SystemError(target);
        }

        swapchainImageViews.push_back(imgv);
    }
}

void OMSwapchainManager::destroy()
{
    for (auto view : swapchainImageViews)
    {
        device.destroyImageView(view, callbacks);
    }
    swapchainImageViews.clear();
    device.destroySwapchainKHR(swapchain, callbacks);
}
}