#include "openminecraft/renderer/vk/om_renderer_layer_vk_swapchainmanager.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"

#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"

#include <SDL3/SDL_video.h>

#include <utility>

using namespace ::vk;
using namespace openminecraft::i18n::res;

namespace openminecraft::renderer::vk::swapchain
{
OMSwapchainManager::OMSwapchainManager(SurfaceKHR surface, std::function<OMSwapchainCap()> fetch,
                                       std::pair<uint32_t, uint32_t> families, Device dev,
                                       AllocationCallbacks callbacks, void *window)
    : families(std::move(families)), fetch(std::move(fetch)), surface(surface), device(dev), callbacks(callbacks),
      window(window)
{
    OMSwapchainManager::reinit();
}

auto OMSwapchainManager::chooseSurfaceFormat(OMSwapchainCap cap) -> SurfaceFormatKHR
{
    for (auto sf : cap.formats)
    {
        if ((sf.format == Format::eB8G8R8A8Srgb || sf.format == Format::eR8G8B8A8Srgb) &&
            sf.colorSpace == ColorSpaceKHR::eSrgbNonlinear)
        {
            return sf;
        }
    }

    return cap.formats[0];
}

auto OMSwapchainManager::chooseExtent(const OMSwapchainCap &cap) -> Extent2D
{
    if (cap.capabilities.currentExtent.width != -1)
    {
        return cap.capabilities.currentExtent;
    }

    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(static_cast<SDL_Window *>(window), &w, &h);

    return VkExtent2D{std::min(cap.capabilities.maxImageExtent.width,
                               std::max(static_cast<uint32_t>(w), cap.capabilities.minImageExtent.width)),
                      std::min(cap.capabilities.maxImageExtent.height,
                               std::max(static_cast<uint32_t>(h), cap.capabilities.minImageExtent.height))};
}

auto OMSwapchainManager::choosePresentMode(const OMSwapchainCap &cap) -> PresentModeKHR
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

    SwapchainCreateInfoKHR createInfo({}, surface, imageCount, form.format, form.colorSpace, extent, 1,
                                      ImageUsageFlagBits::eColorAttachment, {}, {}, {},
                                      supp.capabilities.currentTransform, CompositeAlphaFlagBitsKHR::eOpaque,
                                      choosePresentMode(supp), false, nullptr);

    std::vector<uint32_t> fams = {families.first, families.second};
    if (families.first != families.second)
    {
        createInfo.imageSharingMode = SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = fams.data();
    }
    else
    {
        createInfo.imageSharingMode = SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 1;
        createInfo.pQueueFamilyIndices = fams.data();
    }

    swapchain = device.createSwapchainKHR(createInfo, callbacks);

    auto target = device.getSwapchainImagesKHR(swapchain, &swapchainImageCount, nullptr);
    if (target != Result::eSuccess)
    {
        throw OMRendererException(
            VkErrorTranslate(SystemError(target), "openminecraft.renderer.vk.err.swapchain.images"));
    }

    std::vector<Image> images(swapchainImageCount);
    target = device.getSwapchainImagesKHR(swapchain, &swapchainImageCount, images.data());
    if (target != Result::eSuccess)
    {
        throw OMRendererException(
            VkErrorTranslate(SystemError(target), "openminecraft.renderer.vk.err.swapchain.images"));
    }

    swapchainImages = images;

    for (auto img : images)
    {
        ImageViewCreateInfo createInfo({}, img, ImageViewType::e2D, form.format,
                                       {ComponentSwizzle::eIdentity, ComponentSwizzle::eIdentity,
                                        ComponentSwizzle::eIdentity, ComponentSwizzle::eIdentity},
                                       ImageSubresourceRange(ImageAspectFlagBits::eColor, 0, 1, 0, 1));

        ImageView imgv;
        target = device.createImageView(&createInfo, &callbacks, &imgv);
        if (target != Result::eSuccess)
        {
            throw OMRendererException(
                VkErrorTranslate(SystemError(target), "openminecraft.renderer.vk.err.swapchain.imageview"));
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
} // namespace openminecraft::renderer::vk::swapchain
