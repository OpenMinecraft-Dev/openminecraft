#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_pipeline.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_rendertarget.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_task.hpp"
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
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifdef OM_VULKAN_DYNAMIC
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

#if defined(OM_PLATFORM_WINDOWS) && !defined(OM_VULKAN_DYNAMIC)
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
    throw OMRendererException(VkErrorTranslate(err, id));
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
                 reinterpret_cast<PFN_AllocationFunction>(vkAlloc),
                 reinterpret_cast<PFN_ReallocationFunction>(vkRealloc),
                 reinterpret_cast<PFN_FreeFunction>(vkFree),
                 reinterpret_cast<PFN_InternalAllocationNotification>(vkInternalAlloc),
                 reinterpret_cast<PFN_InternalFreeNotification>(vkInternalFree)};

    {
#ifdef OM_VULKAN_DYNAMIC
        auto vkGetInstanceProcAddr = loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
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
        throw OMRendererException(extResult.unwrap_err());
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
        throw OMRendererException(instanceResult.unwrap_err());
    }
    }

    auto sdlLoadingResult = sdlVulkanLoading();
    switch (sdlLoadingResult.type)
    {
    case Ok: {
        break;
    }
    case Err: {
        throw OMRendererException(sdlLoadingResult.unwrap_err());
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
        throw OMRendererException(phydevResult.unwrap_err());
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
        throw OMRendererException(deviceCreationResult.unwrap_err());
    }
    }

    auto queuefetch = deviceQueueFetch();
    switch (queuefetch.type)
    {
    case Ok: {
        break;
    }
    case Err: {
        throw OMRendererException(queuefetch.unwrap_err());
    }
    }

    try
    {
        swapchainManager = std::make_shared<swapchain::OMSwapchainManager>(
            surface, [&]() -> swapchain::OMSwapchainCap { return getSwapchainCap(); }, queueFamilyIndex, logicalDevice,
            allocator, window);
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.swp"));
    }

    memProps = physicalDevice.getMemoryProperties();

    try
    {
        tempCommandPool = logicalDevice.createCommandPool(CommandPoolCreateInfo({}, queueFamilyIndex.first), allocator);

        defaultTarget = this->createRenderTarget();
        defaultTarget->build();
        defaultDepthBuffer = this->allocateTexture(swapchainManager->extent.width, swapchainManager->extent.height,
                                                   common::Dim2, common::Depth);

        for (int i = 0; i < framesInFlight; i++)
        {
            frameSyncs.push_back(
                {{logicalDevice.createSemaphore(SemaphoreCreateInfo(), allocator)},
                 logicalDevice.createFence(FenceCreateInfo(FenceCreateFlagBits::eSignaled), allocator)});
        }

        for (int i = 0; i < swapchainManager->swapchainImageViews.size(); i++)
        {
            frameRenderSemaphores.push_back(logicalDevice.createSemaphore(SemaphoreCreateInfo(), allocator));
        }
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.renderer"));
    }
}

void OMRendererVk::rebuildDefaults()
{
    try
    {
        for (auto &fb : defaultFramebuffers)
        {
            logicalDevice.destroyFramebuffer(fb, allocator);
        }
        defaultFramebuffers.clear();
        for (auto &cb : defaultCommandBuffers)
        {
            for (auto &cbn : cb.second)
            {
                logicalDevice.freeCommandBuffers(tempCommandPool, cbn);
            }
        }
        defaultCommandBuffers.clear();

        for (auto img : swapchainManager->swapchainImageViews)
        {
            std::vector ii = {img, reinterpret_cast<OMRendererTextureVk *>(defaultDepthBuffer)->imageView};
            defaultFramebuffers.push_back(logicalDevice.createFramebuffer(
                FramebufferCreateInfo(
                    {}, reinterpret_cast<OMRendererRenderTargetVk *>(getDefaultRenderTarget())->renderPass, ii,
                    swapchainManager->extent.width, swapchainManager->extent.height, 1),
                allocator));
        }

        for (auto task : tasks)
        {
            if (!reinterpret_cast<OMRendererTaskVk *>(task.second)->isOnDefault())
            {
                continue;
            }
            for (auto defaultFramebuffer : defaultFramebuffers)
            {
                auto commandBuffer = logicalDevice.allocateCommandBuffers(
                    CommandBufferAllocateInfo(tempCommandPool, CommandBufferLevel::ePrimary, 1))[0];

                commandBuffer.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eSimultaneousUse));
                std::vector test = {ClearValue({0.0f, 0.0f, 0.0f, 0.0f}), ClearValue({1.0f, 0})};
                commandBuffer.beginRenderPass(
                    RenderPassBeginInfo(
                        reinterpret_cast<OMRendererRenderTargetVk *>(getDefaultRenderTarget())->renderPass,
                        defaultFramebuffer, Rect2D(Offset2D(0, 0), swapchainManager->extent), test),
                    SubpassContents::eSecondaryCommandBuffers);

                commandBuffer.executeCommands(reinterpret_cast<OMRendererTaskVk *>(task.second)->commandBuffer);
                commandBuffer.endRenderPass();
                commandBuffer.end();

                defaultCommandBuffers[task.second].push_back(commandBuffer);
                // defaultCommandBuffers.push_back(commandBuffer);
            }
        }

        for (auto &sync : frameSyncs)
        {
            while (sync.pipelineSemaphores.size() != tasks.size())
            {
                if (sync.pipelineSemaphores.size() < tasks.size())
                {
                    sync.pipelineSemaphores.push_back(logicalDevice.createSemaphore(SemaphoreCreateInfo(), allocator));
                }
                else
                {
                    logicalDevice.destroySemaphore(*sync.pipelineSemaphores.rbegin(), allocator);
                    sync.pipelineSemaphores.pop_back();
                }
            }
        }
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.defaults"));
    }
}

void OMRendererVk::baseInit()
{
    for (auto h : handlers)
    {
        h->submitTasks();
    }
    buildTaskGraph();
    rebuildDefaults();
}

auto OMRendererVk::createTask() -> common::OMRendererTask *
{
    return new OMRendererTaskVk(this);
}

auto OMRendererVk::createPipeline() -> common::OMRendererPipeline *
{
    return new OMRendererPipelineVk(this);
}

auto OMRendererVk::allocateBuffer(common::OMBufferUsage usage, uint64_t length) -> common::OMRendererBuffer *
{
    return new OMRendererBufferVk(usage, length, this);
}

auto OMRendererVk::allocateTexture(uint64_t width, uint64_t height, common::OMTextureType type,
                                   common::OMTextureArrangement arr) -> common::OMRendererTexture *
{
    return new OMRendererTextureVk(width, height, type, arr, this);
}
auto OMRendererVk::createRenderTarget() -> common::OMRendererRenderTarget *
{
    return new OMRendererRenderTargetVk(this);
}
auto OMRendererVk::getDefaultRenderTarget() -> common::OMRendererRenderTarget *
{
    return this->defaultTarget;
}
auto OMRendererVk::getExtent() const -> glm::vec2
{
    return {static_cast<float>(swapchainManager->extent.width), static_cast<float>(swapchainManager->extent.height)};
}

void OMRendererVk::render()
{
    if (needRebuild)
    {
        goto rebuild;
    }
    try
    {
        for (auto r : handlers)
        {
            r->beforeFrame();
        }
        auto result = logicalDevice.waitForFences(1, &frameSyncs[thisFrame].inFlightFence, true,
                                                  std::numeric_limits<uint64_t>::max());
        if (result != Result::eSuccess)
        {
            throw OMRendererException(VkErrorTranslate(SystemError(result), "openminecraft.renderer.vk.err.waitfence"));
        }

        auto [nxtRes, imageIndex] =
            logicalDevice.acquireNextImageKHR(swapchainManager->swapchain, std::numeric_limits<uint64_t>::max(),
                                              frameSyncs[thisFrame].imageAvailable(), {});
        if (nxtRes != Result::eSuccess)
        {
            if (nxtRes == Result::eSuboptimalKHR || nxtRes == Result::eErrorOutOfDateKHR)
            {
                goto rebuild;
            }
            throw OMRendererException(VkErrorTranslate(SystemError(result), "openminecraft.renderer.vk.err.nextimage"));
        }

        if (inflights.count(imageIndex) > 0)
        {
            auto result = logicalDevice.waitForFences(1, &inflights[imageIndex].inFlightFence, true,
                                                      std::numeric_limits<uint64_t>::max());
            if (result != Result::eSuccess)
            {
                throw OMRendererException(
                    VkErrorTranslate(SystemError(result), "openminecraft.renderer.vk.err.waitfence"));
            }
        }
        inflights[imageIndex] = frameSyncs[thisFrame];

        result = logicalDevice.resetFences(1, &frameSyncs[thisFrame].inFlightFence);
        if (result != Result::eSuccess)
        {
            throw OMRendererException(
                VkErrorTranslate(SystemError(result), "openminecraft.renderer.vk.err.resetfence"));
        }

        for (int i = 0; i < layeredTasks.size(); ++i)
        {
            std::cout << "wait " << i << std::endl;
            std::cout << "signal " << i + 1 << std::endl;
        }
        std::cout << frameSyncs[thisFrame].pipelineSemaphores.size() << " seps" << std::endl;

        std::vector<CommandBuffer> cmdBuffers = {};
        for (auto tsk : tasks)
        {
            auto tt = reinterpret_cast<OMRendererTaskVk *>(tsk.second);
            if (!tt->isOnDefault())
            {
                cmdBuffers.push_back(tt->commandBuffer);
            }
            else
            {
                cmdBuffers.push_back(defaultCommandBuffers[tt][imageIndex]);
            }
        }

        auto frmSep = frameRenderSemaphores[imageIndex];
        const PipelineStageFlags msk = PipelineStageFlagBits::eColorAttachmentOutput;
        SubmitInfo submitInfo(1, &frameSyncs[thisFrame].pipelineSemaphores[0], &msk, cmdBuffers.size(),
                              cmdBuffers.data(), 1, &frmSep);

        queues.first.submit(submitInfo, frameSyncs[thisFrame].inFlightFence);

        PresentInfoKHR presentInfo(1, &frmSep, 1, &swapchainManager->swapchain, &imageIndex);

        result = queues.second.presentKHR(presentInfo);
        if (result != Result::eSuccess)
        {
            throw OMRendererException(
                VkErrorTranslate(SystemError(result), "openminecraft.renderer.vk.err.queuepresent"));
        }

        thisFrame = (thisFrame + 1) % framesInFlight;
        for (auto r : handlers)
        {
            r->afterFrame();
        }
        return;
    }
    catch (SystemError &e)
    {
        if (e.code().value() == VK_SUBOPTIMAL_KHR || e.code().value() == VK_ERROR_OUT_OF_DATE_KHR)
        {
            goto rebuild;
        }
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.queuepresent"));
    }

rebuild:
    try
    {
        logicalDevice.waitIdle();
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.wait"));
    }
    swapchainManager->destroy();
    swapchainManager->reinit();

    delete defaultDepthBuffer;
    defaultDepthBuffer = this->allocateTexture(swapchainManager->extent.width, swapchainManager->extent.height,
                                               common::Dim2, common::Depth);

    this->clearTasks();
    for (auto r : handlers)
    {
        r->submitTasks();
    }
    buildTaskGraph();
    rebuildDefaults();
    needRebuild = false;

    for (auto r : handlers)
    {
        r->afterFrame();
    }
}

void OMRendererVk::requestResize()
{
    this->needRebuild = true;
}

auto OMRendererVk::getSwapchainCap() -> swapchain::OMSwapchainCap
{
    try
    {
        return swapchain::OMSwapchainCap{physicalDevice.getSurfaceCapabilitiesKHR(surface),
                                         physicalDevice.getSurfaceFormatsKHR(surface),
                                         physicalDevice.getSurfacePresentModesKHR(surface)};
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.swapchaincap"));
    }
}

auto OMRendererVk::deviceQueueFetch() -> OMResult<std::any, std::string>
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

auto OMRendererVk::deviceCreation() -> OMResult<Device, std::string>
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
        return OMResult<Device, std::string>::ok(
            physicalDevice.createDevice(DeviceCreateInfo({}, qis, {}, ext, &fea), allocator));
    }
    catch (SystemError &e)
    {
        logger->error(VkErrorTranslate(e, "openminecraft.renderer.vk.err.dev"));
        return OMResult<Device, std::string>::err(VkErrorTranslate(e, "openminecraft.renderer.vk.err.dev"));
    }
}

auto OMRendererVk::sdlVulkanLoading() -> OMResult<std::any, std::string>
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
                             reinterpret_cast<VkAllocationCallbacks *>(&allocator), &sf);
    surface = SurfaceKHR(sf);

    return OMResult<std::any, std::string>::ok(nullptr);
}
auto OMRendererVk::deviceSelection(std::function<int(std::vector<std::string>)> dev)
    -> OMResult<PhysicalDevice, std::string>
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
            d.emplace_back(n.data());
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
auto OMRendererVk::instanceCreation(AppInfo info, std::vector<const char *> exts) -> OMResult<Instance, std::string>
{
    try
    {
        ApplicationInfo appInfo(info.appName.c_str(), info.appVer.toVKVersion(), info.engineName.c_str(),
                                info.engineVer.toVKVersion(), info.minApiVersion.toVKApiVersion());
        std::vector<const char *> l;
        validationLayer->attach(&l);
        auto i = createInstance(InstanceCreateInfo{{}, &appInfo, l, exts, &validationLayer->createInfo}, allocator);
        logger->info(translate("openminecraft.renderer.vk.instance", info.appName, info.appVer.toString(),
                               info.engineName, info.engineVer.toString(), info.minApiVersion.toString()));
#ifdef OM_VULKAN_DYNAMIC
        VULKAN_HPP_DEFAULT_DISPATCHER.init(i);
#endif

        validationLayer->ifEnable([&]() -> void {
            auto r = i.createDebugReportCallbackEXT(&validationLayer->callbackInfo, &allocator, &reportCallback);
            if (r != Result::eSuccess)
            {
                throw OMRendererException(VkErrorTranslate(SystemError(r), "openminecraft.renderer.vk.err.dbg"));
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
auto OMRendererVk::fetchRequiredExtensions() -> OMResult<std::vector<const char *>, std::string>
{
    try
    {
        std::vector<const char *> exts;
        unsigned int extcount = 0;
        const char *const *ext = SDL_Vulkan_GetInstanceExtensions(&extcount);
        logger->info(translate("openminecraft.renderer.vk.ext", extcount));
        for (int i = 0; i < extcount; i++)
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
auto vkAlloc(void *, size_t size, size_t align, VkSystemAllocationScope s) -> void *
{
    void *p = malloc((size / align + 1) * align);
    mem::castorice::rec({mem::castorice::Allocation, p, mem::castorice::heapSize(p), "vulkan"});
    return p;
}
auto vkRealloc(void *, void *o, size_t size, size_t align, VkSystemAllocationScope s) -> void *
{
    void *p;
    auto newsize = (size / align + 1) * align;
    if (o != nullptr)
    {
        mem::castorice::rec({mem::castorice::Free, o, mem::castorice::heapSize(o), "vulkan"});
        p = realloc(o, newsize);
        mem::castorice::rec({mem::castorice::Allocation, p, mem::castorice::heapSize(p), "vulkan"});
    }
    else
    {
        p = malloc(newsize);
        mem::castorice::rec({mem::castorice::Allocation, p, mem::castorice::heapSize(p), "vulkan"});
    }
    return p;
}
void vkFree(void *, void *p)
{
    if (p == nullptr)
        return;
    mem::castorice::rec({mem::castorice::Free, p, mem::castorice::heapSize(p), "vulkan"});
    free(p);
}
void vkInternalAlloc(void *, size_t size, VkInternalAllocationType t, VkSystemAllocationScope s)
{
    mem::castorice::rec({mem::castorice::Allocation, nullptr, size, "vulkan_internal"});
}
void vkInternalFree(void *, size_t size, VkInternalAllocationType t, VkSystemAllocationScope s)
{
    mem::castorice::rec({mem::castorice::Free, nullptr, size, "vulkan_internal"});
}
OMRendererVk::~OMRendererVk()
{
    try
    {
        logicalDevice.waitIdle();
        for (auto sync : frameSyncs)
        {
            for (auto s : sync.pipelineSemaphores)
            {
                logicalDevice.destroySemaphore(s, allocator);
            }
            logicalDevice.destroyFence(sync.inFlightFence, allocator);
        }
        for (auto sep : frameRenderSemaphores)
        {
            logicalDevice.destroySemaphore(sep, allocator);
        }

        delete defaultDepthBuffer;
        for (auto &fb : defaultFramebuffers)
        {
            logicalDevice.destroyFramebuffer(fb, allocator);
        }
        for (auto &cb : defaultCommandBuffers)
        {
            for (auto &cbn : cb.second)
            {
                logicalDevice.freeCommandBuffers(tempCommandPool, cbn);
            }
        }
        this->clearTasks();
        logicalDevice.destroyCommandPool(tempCommandPool, allocator);
        delete defaultTarget;
        handlers.clear();

        swapchainManager->destroy();
        validationLayer->ifEnable(
            [&]() -> void { instance.destroyDebugReportCallbackEXT(reportCallback, &allocator); });

        logicalDevice.destroy(allocator);
        instance.destroySurfaceKHR(surface, allocator);
        instance.destroy(allocator);
        SDL_Vulkan_UnloadLibrary();
    }
    catch (SystemError &e)
    {
    }
}
auto OMRendererVk::driver() -> std::string
{
    try
    {
        return physicalDevice.getProperties().deviceName;
    }
    catch (SystemError &e)
    {
        return "<unknown>";
    }
}
} // namespace openminecraft::renderer::vk
