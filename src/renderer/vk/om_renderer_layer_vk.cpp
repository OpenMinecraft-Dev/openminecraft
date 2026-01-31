#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_texture.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_validation.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <SDL3/SDL_vulkan.h>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifdef OM_VULKAN_DYNAMIC
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

#ifdef OM_PLATFORM_WINDOWS
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                              const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator,
                                                              VkDebugUtilsMessengerEXT *pMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDrbugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pMessenger);
    }
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
                                                           const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, messenger, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(VkInstance instance,
                                                              const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator,
                                                              VkDebugReportCallbackEXT *pCallback)
{
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    }
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback,
                                                           const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr)
    {
        func(instance, callback, pAllocator);
    }
}
#endif

using namespace vk;
using namespace openminecraft::util;
using openminecraft::i18n::res::translate;
#define VkErrLogAndThrow(err, id)                                                                                      \
    logger->error(VkErrorTranslate(err, id));                                                                          \
    throw std::runtime_error(VkErrorTranslate(err, id));
namespace openminecraft::renderer::vk
{
#ifdef OM_VULKAN_DYNAMIC
detail::DynamicLoader loader;
#endif
OMRendererVk::OMRendererVk(AppInfo info, std::function<int(std::vector<std::string>)> dev, void *window)
    : OMRenderer(info, window)
{
    logger = std::make_shared<log::OMLogger>("OMRendererVk", this);

    allocator = {nullptr,
                 (PFN_AllocationFunction)vkAlloc,
                 (PFN_ReallocationFunction)vkRealloc,
                 (PFN_FreeFunction)vkFree,
                 (PFN_InternalAllocationNotification)vkInternalAlloc,
                 (PFN_InternalFreeNotification)vkInternalFree};

    {
#ifdef OM_VULKAN_DYNAMIC
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
            loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
#endif
    }

    auto extResult = fetchRequiredExtensions();
    std::vector<const char *> exts;

    switch (extResult.type)
    {
    case Ok: {
        exts = extResult.unwrap();
        break;
    }
    case Err: {
        throw std::runtime_error(extResult.unwrap_err());
    }
    }

    auto instanceResult = instanceCreation(info, exts);
    switch (instanceResult.type)
    {
    case Ok: {
        instance = instanceResult.unwrap();
        break;
    }
    case Err: {
        throw std::runtime_error(instanceResult.unwrap_err());
    }
    }

    auto sdlLoadingResult = sdlVulkanLoading();
    switch (sdlLoadingResult.type)
    {
    case Ok: {
        break;
    }
    case Err: {
        throw std::runtime_error(sdlLoadingResult.unwrap_err());
    }
    }

    auto phydevResult = deviceSelection(dev);
    switch (phydevResult.type)
    {
    case Ok: {
        physicalDevice = phydevResult.unwrap();
        break;
    }
    case Err: {
        throw std::runtime_error(phydevResult.unwrap_err());
    }
    }

    auto deviceCreationResult = deviceCreation();
    switch (deviceCreationResult.type)
    {
    case Ok: {
        logicalDevice = deviceCreationResult.unwrap();
        break;
    }
    case Err: {
        throw std::runtime_error(deviceCreationResult.unwrap_err());
    }
    }

    auto queuefetch = deviceQueueFetch();
    switch (queuefetch.type)
    {
    case Ok: {
        break;
    }
    case Err: {
        throw std::runtime_error(queuefetch.unwrap_err());
    }
    }

    try
    {
        swapchainManager = std::make_shared<swapchain::OMSwapchainManager>(
            surface, [&]() { return getSwapchainCap(); }, queueFamilyIndex, logicalDevice, allocator, window);
    }
    catch (SystemError &e)
    {
        throw std::runtime_error(VkErrorTranslate(e, "openminecraft.renderer.vk.err.swp"));
    }

    memProps = physicalDevice.getMemoryProperties();

    try
    {
        tempCommandPool = logicalDevice.createCommandPool(CommandPoolCreateInfo({}, queueFamilyIndex.first), allocator);

        testRenderer = std::make_shared<test::OMTestRenderer>(this);

        for (int i = 0; i < framesInFlight; i++)
        {
            frameSyncs.push_back(
                {logicalDevice.createSemaphore(SemaphoreCreateInfo(), allocator),
                 logicalDevice.createFence(FenceCreateInfo(FenceCreateFlagBits::eSignaled), allocator)});
        }

        for (int i = 0; i < swapchainManager->swapchainImageViews.size(); i++)
        {
            frameRenderSemaphores.push_back(logicalDevice.createSemaphore(SemaphoreCreateInfo(), allocator));
        }
    }
    catch (SystemError &e)
    {
        throw std::runtime_error(VkErrorTranslate(e, "openminecraft.renderer.vk.err.renderer"));
    }
}

common::OMRendererBuffer *OMRendererVk::allocateBuffer(common::OMBufferUsage usage, uint64_t length)
{
    return new OMRendererBufferVk(usage, length, this);
}

common::OMRendererTexture *OMRendererVk::allocateTexture(uint64_t width, uint64_t height, common::OMTextureType type,
                                                         common::OMTextureArrangement arr)
{
    return new OMRendererTextureVk(width, height, type, arr, this);
}
glm::vec2 OMRendererVk::getExtent() const
{
    return {static_cast<float>(swapchainManager->extent.width), static_cast<float>(swapchainManager->extent.height)};
}

void OMRendererVk::render()
{
    if (needRebuild)
    {
        goto reb;
    }
    try
    {
        testRenderer->updateUniform();
        auto result = logicalDevice.waitForFences(1, &frameSyncs[thisFrame].inFlightFence, true,
                                                  std::numeric_limits<uint64_t>::max());
        if (result != Result::eSuccess)
        {
            throw SystemError(result);
        }

        auto [nxtRes, imageIndex] =
            logicalDevice.acquireNextImageKHR(swapchainManager->swapchain, std::numeric_limits<uint64_t>::max(),
                                              frameSyncs[thisFrame].imageAvailableSemaphore, {});
        if (nxtRes != Result::eSuccess)
        {
            throw SystemError(nxtRes);
        }

        if (inflights.count(imageIndex) > 0)
        {
            auto result = logicalDevice.waitForFences(1, &inflights[imageIndex].inFlightFence, true,
                                                      std::numeric_limits<uint64_t>::max());
            if (result != Result::eSuccess)
            {
                throw SystemError(result);
            }
        }
        inflights[imageIndex] = frameSyncs[thisFrame];

        result = logicalDevice.resetFences(1, &frameSyncs[thisFrame].inFlightFence);
        if (result != Result::eSuccess)
        {
            throw SystemError(result);
        }

        const PipelineStageFlags msk = PipelineStageFlagBits::eColorAttachmentOutput;
        SubmitInfo submitInfo(1, &frameSyncs[thisFrame].imageAvailableSemaphore, &msk, 1,
                              &testRenderer->commandBuffers[imageIndex], 1, &frameRenderSemaphores[imageIndex]);

        queues.first.submit(submitInfo, frameSyncs[thisFrame].inFlightFence);

        PresentInfoKHR presentInfo(1, &frameRenderSemaphores[imageIndex], 1, &swapchainManager->swapchain, &imageIndex);

        result = queues.second.presentKHR(presentInfo);
        if (result != Result::eSuccess)
        {
            throw SystemError(result);
        }

        thisFrame = (thisFrame + 1) % framesInFlight;
        return;
    }
    catch (SystemError &e)
    {
        if (e.code().value() == VK_SUBOPTIMAL_KHR || e.code().value() == VK_ERROR_OUT_OF_DATE_KHR)
        {
            goto reb;
        }
        throw;
    }

reb:
    logicalDevice.waitIdle();
    swapchainManager->destroy();
    swapchainManager->reinit();
    testRenderer->reinit();
    needRebuild = false;
}

swapchain::OMSwapchainCap OMRendererVk::getSwapchainCap()
{
    return swapchain::OMSwapchainCap{physicalDevice.getSurfaceCapabilitiesKHR(surface),
                                     physicalDevice.getSurfaceFormatsKHR(surface),
                                     physicalDevice.getSurfacePresentModesKHR(surface)};
}

OMResult<std::any, std::string> OMRendererVk::deviceQueueFetch()
{
    try
    {
        queues = std::make_pair(logicalDevice.getQueue(queueFamilyIndex.first, 0),
                                logicalDevice.getQueue(queueFamilyIndex.second, 0));
    }
    catch (SystemError &e)
    {
        logger->error(VkErrorTranslate(e, "openminecraft.renderer.vk.err.devqueue"));
        return OMResult<std::any, std::string>::err(VkErrorTranslate(e, "openminecraft.renderer.vk.err.devqueue"));
    }

    return OMResult<std::any, std::string>::ok(0);
}

OMResult<Device, std::string> OMRendererVk::deviceCreation()
{
    try
    {
        std::vector<DeviceQueueCreateInfo> qis;
        auto f = 1.f;
        qis.push_back(DeviceQueueCreateInfo({}, queueFamilyIndex.first, 1, &f));
        if (queueFamilyIndex.first != queueFamilyIndex.second)
        {
            qis.push_back(DeviceQueueCreateInfo({}, queueFamilyIndex.second, 1, &f));
        }
        auto fea = physicalDevice.getFeatures();

        std::vector ext{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        return OMResult<Device, std::string>::ok(physicalDevice.createDevice(DeviceCreateInfo({}, qis, {}, ext, &fea)));
    }
    catch (SystemError &e)
    {
        logger->error(VkErrorTranslate(e, "openminecraft.renderer.vk.err.dev"));
        return OMResult<Device, std::string>::err(VkErrorTranslate(e, "openminecraft.renderer.vk.err.dev"));
    }
}

OMResult<std::any, std::string> OMRendererVk::sdlVulkanLoading()
{
#define errRet(a)                                                                                                      \
    logger->error("{}", a);                                                                                            \
    return util::OMResult<std::any, std::string>::err(translate("openminecraft.renderer.vk.sdl.err", a));

    auto d0 = SDL_GetError();
    if (strlen(d0))
    {
        errRet(d0);
    }
    logger->info(translate("openminecraft.renderer.vk.sdl.vulkan"));
    SDL_Vulkan_LoadLibrary(nullptr);
    auto d = SDL_GetError();
    if (strlen(d))
    {
        errRet(d);
    }

    VkSurfaceKHR sf;
    SDL_Vulkan_CreateSurface(static_cast<SDL_Window *>(window), instance,
                             reinterpret_cast<const VkAllocationCallbacks *>(&allocator), &sf);
    surface = SurfaceKHR(sf);

    return OMResult<std::any, std::string>::ok(nullptr);
}
OMResult<PhysicalDevice, std::string> OMRendererVk::deviceSelection(std::function<int(std::vector<std::string>)> dev)
{
    try
    {
        auto phyDev = instance.enumeratePhysicalDevices();
        std::vector<std::string> d;
        logger->info(translate("openminecraft.renderer.vk.devcount", phyDev.size()));
        int id = 0;
        auto ids = dev(d);
        PhysicalDevice devtarget;
        for (auto pdev : phyDev)
        {
            auto n = pdev.getProperties().deviceName;
            logger->info("{} {}", id == ids ? "->" : "  ", n.data());
            if (id == ids)
            {
                devtarget = pdev;
            }
            d.push_back(n.data());
            id++;
        }

        auto fmly = devtarget.getQueueFamilyProperties();

        auto gf = -1;
        auto pf = -1;

        int idx = 0;
        for (auto i : fmly)
        {
            if (i.queueFlags & QueueFlagBits::eGraphics)
            {
                gf = idx;
            }

            if (devtarget.getSurfaceSupportKHR(idx, surface))
            {
                pf = idx;
            }
            idx++;
        }

        queueFamilyIndex = std::make_pair(gf, pf);

        if (gf == -1 || pf == -1)
        {
            return OMResult<PhysicalDevice, std::string>::err(translate("openminecraft.renderer.vk.err.family"));
        }

        return OMResult<PhysicalDevice, std::string>::ok(devtarget);
    }
    catch (SystemError &e)
    {
        logger->error(VkErrorTranslate(e, "openminecraft.renderer.vk.err.phydev"));
        return OMResult<PhysicalDevice, std::string>::err(VkErrorTranslate(e, "openminecraft.renderer.vk.err.phydev"));
    }
}
OMResult<Instance, std::string> OMRendererVk::instanceCreation(AppInfo info, std::vector<const char *> exts)
{
    try
    {
        ApplicationInfo appInfo(info.appName.c_str(), info.appVer.toVKVersion(), info.engineName.c_str(),
                                info.engineVer.toVKVersion(), info.minApiVersion.toVKApiVersion());
        std::vector<const char *> l;
        validationLayer->attach(&l);
        auto i = createInstance(InstanceCreateInfo{{}, &appInfo, l, exts, &validationLayer->createInfo});
        logger->info(translate("openminecraft.renderer.vk.instance", info.appName, info.appVer.toString(),
                               info.engineName, info.engineVer.toString(), info.minApiVersion.toString()));
#ifdef OM_VULKAN_DYNAMIC
        VULKAN_HPP_DEFAULT_DISPATCHER.init(i);
#endif

        validationLayer->ifEnable([&]() {
            auto r = i.createDebugReportCallbackEXT(&validationLayer->callbackInfo, &allocator, &reportCallback);
            if (r != Result::eSuccess)
            {
                throw SystemError(r);
            }
        });

        return OMResult<Instance, std::string>::ok(i);
    }
    catch (SystemError &e)
    {
        logger->error(VkErrorTranslate(e, "openminecraft.renderer.vk.err.instance"));
        return OMResult<Instance, std::string>::err(VkErrorTranslate(e, "openminecraft.renderer.vk.err.instance"));
    }
}
OMResult<std::vector<const char *>, std::string> OMRendererVk::fetchRequiredExtensions()
{
    try
    {
        std::vector<const char *> exts;
        unsigned int extcount = 0;
        const char *const *ext = SDL_Vulkan_GetInstanceExtensions(&extcount);
        logger->info(translate("openminecraft.renderer.vk.ext", extcount));
        for (int i = 0; i < 2; i++)
        {
            logger->info(ext[i]);
            exts.push_back(ext[i]);
        }

        auto layers = enumerateInstanceLayerProperties();
        logger->info(translate("openminecraft.renderer.vk.layercount", layers.size()));
        for (auto l : layers)
        {
            logger->info(translate("openminecraft.renderer.vk.layerdata", l.layerName.data(), l.description.data(),
                                   Version(l.implementationVersion).toString(), Version(l.specVersion).toString()));
        }
        validationLayer = std::make_shared<validation::OMRendererVkValidation>(layers);
        validationLayer->attachExts(&exts);
        return OMResult<std::vector<const char *>, std::string>::ok(exts);
    }
    catch (SystemError &e)
    {
        logger->error(VkErrorTranslate(e, "openminecraft.renderer.vk.err.preinstance"));
        return OMResult<std::vector<const char *>, std::string>::err(
            VkErrorTranslate(e, "openminecraft.renderer.vk.err.preinstance"));
    }
}
void *vkAlloc(void *, size_t size, size_t align, VkSystemAllocationScope s)
{
    void *p = malloc((size / align + 1) * align);
    mem::castorice::rec({mem::castorice::Allocation, p, mem::castorice::heapSize(p), OM_MEM_VULKAN});
    return p;
}
void *vkRealloc(void *, void *o, size_t size, size_t align, VkSystemAllocationScope s)
{
    void *p;
    if (o != nullptr)
    {
        mem::castorice::rec({mem::castorice::Free, o, mem::castorice::heapSize(o), OM_MEM_VULKAN});
        p = realloc(o, (size / align + 1) * align);
        mem::castorice::rec({mem::castorice::Allocation, p, mem::castorice::heapSize(p), OM_MEM_VULKAN});
    }
    else
    {
        p = malloc((size / align + 1) * align);
        mem::castorice::rec({mem::castorice::Allocation, p, mem::castorice::heapSize(p), OM_MEM_VULKAN});
    }
    return p;
}
void vkFree(void *, void *p)
{
    if (p == nullptr)
        return;
    mem::castorice::rec({mem::castorice::Free, p, mem::castorice::heapSize(p), OM_MEM_VULKAN});
    free(p);
}
void vkInternalAlloc(void *, size_t size, VkInternalAllocationType t, VkSystemAllocationScope s)
{
    mem::castorice::rec({mem::castorice::Allocation, nullptr, size, OM_MEM_VULKAN_INTERNAL});
}
void vkInternalFree(void *, size_t size, VkInternalAllocationType t, VkSystemAllocationScope s)
{
    mem::castorice::rec({mem::castorice::Free, nullptr, size, OM_MEM_VULKAN_INTERNAL});
}
OMRendererVk::~OMRendererVk()
{
}
void OMRendererVk::destroy()
{
    for (auto sync : frameSyncs)
    {
        logicalDevice.destroySemaphore(sync.imageAvailableSemaphore, allocator);
        logicalDevice.destroyFence(sync.inFlightFence, allocator);
    }
    for (auto sep : frameRenderSemaphores)
    {
        logicalDevice.destroySemaphore(sep, allocator);
    }
    logicalDevice.destroyCommandPool(tempCommandPool, allocator);
    testRenderer->destroy();
    swapchainManager->destroy();
    SDL_Vulkan_DestroySurface(instance, VkSurfaceKHR(surface),
                              reinterpret_cast<const VkAllocationCallbacks *>(&allocator));
    logicalDevice.destroy(allocator);
    validationLayer->ifEnable([&]() { instance.destroyDebugReportCallbackEXT(reportCallback, &allocator); });
    instance.destroy(allocator);
    SDL_Vulkan_UnloadLibrary();
}
std::string OMRendererVk::driver()
{
    return physicalDevice.getProperties().deviceName;
}
} // namespace openminecraft::renderer::vk
