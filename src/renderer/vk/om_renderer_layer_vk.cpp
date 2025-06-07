#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_validation.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include <SDL3/SDL_vulkan.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
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
#endif

using namespace vk;
using openminecraft::i18n::res::translate;
#define VkErrLogAndThrow(err, id)                                                                                      \
    logger->error(VkErrorTranslate(err, id));                                                                          \
    throw std::runtime_error(VkErrorTranslate(err, id));
namespace openminecraft::renderer::vk
{
#ifdef OM_VULKAN_DYNAMIC
detail::DynamicLoader loader;
#endif
OMRendererVk::OMRendererVk(AppInfo info, std::function<int(std::vector<std::string>)> dev) : OMRenderer(info)
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
    case util::Ok: {
        exts = extResult.unwrap();
        break;
    }
    case util::Err: {
        throw std::runtime_error(extResult.unwrap_err());
    }
    }

    auto instanceResult = instanceCreation(info, exts);
    switch (instanceResult.type)
    {
    case util::Ok: {
        instance = instanceResult.unwrap();
        break;
    }
    case util::Err: {
        throw std::runtime_error(instanceResult.unwrap_err());
    }
    }

    auto phydevResult = deviceSelection(dev);
    switch (phydevResult.type)
    {
    case util::Ok: {
        physicalDevice = phydevResult.unwrap();
        break;
    }
    case util::Err: {
        throw std::runtime_error(phydevResult.unwrap_err());
    }
    }

    auto sdlLoadingResult = sdlVulkanLoading();
    switch (sdlLoadingResult.type)
    {
    case util::Ok: {
        break;
    }
    case util::Err: {
        throw std::runtime_error(sdlLoadingResult.unwrap_err());
    }
    }
}
util::OMResult<std::any, std::string> OMRendererVk::sdlVulkanLoading()
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
    return util::OMResult<std::any, std::string>::ok(nullptr);
}
util::OMResult<::vk::PhysicalDevice, std::string> OMRendererVk::deviceSelection(
    std::function<int(std::vector<std::string>)> dev)
{
    try
    {
        auto phyDev = instance.enumeratePhysicalDevices();
        std::vector<std::string> d;
        logger->info(translate("openminecraft.renderer.vk.devcount", phyDev.size()));
        int id = 0;
        auto ids = dev(d);
        for (auto pdev : phyDev)
        {
            auto n = pdev.getProperties().deviceName;
            logger->info("{} {}", id == ids ? "->" : "  ", n.data());
            d.push_back(n.data());
            id++;
        }
        return util::OMResult<::vk::PhysicalDevice, std::string>::ok(phyDev[ids]);
    }
    catch (SystemError e)
    {
        logger->error(VkErrorTranslate(e, "openminecraft.renderer.vk.err.phydev"));
        return util::OMResult<::vk::PhysicalDevice, std::string>::err(
            VkErrorTranslate(e, "openminecraft.renderer.vk.err.phydev"));
    }
}
util::OMResult<::vk::Instance, std::string> OMRendererVk::instanceCreation(AppInfo info, std::vector<const char *> exts)
{
    try
    {
        ApplicationInfo appInfo(info.appName.c_str(), info.appVer.toVKVersion(), info.engineName.c_str(),
                                info.engineVer.toVKVersion(), info.minApiVersion.toVKApiVersion());
        std::vector<const char *> l;
        validationLayer->attach(&l);
        auto i = createInstance({InstanceCreateFlags(), &appInfo, (uint32_t)l.size(), l.data(), (uint32_t)exts.size(),
                                 exts.data(), validationLayer->createInfo},
                                allocator);
        logger->info(translate("openminecraft.renderer.vk.instance", info.appName, info.appVer.toString(),
                               info.engineName, info.engineVer.toString(), info.minApiVersion.toString()));
#ifdef OM_VULKAN_DYNAMIC
        VULKAN_HPP_DEFAULT_DISPATCHER.init(i);
#endif

        validationLayer->ifEnable(
            [&]() { messenger = i.createDebugUtilsMessengerEXT(validationLayer->createInfo, allocator); });
        return util::OMResult<::vk::Instance, std::string>::ok(i);
    }
    catch (SystemError e)
    {
        logger->error(VkErrorTranslate(e, "openminecraft.renderer.vk.err.instance"));
        return util::OMResult<::vk::Instance, std::string>::err(
            VkErrorTranslate(e, "openminecraft.renderer.vk.err.instance"));
    }
}
util::OMResult<std::vector<const char *>, std::string> OMRendererVk::fetchRequiredExtensions()
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
                                   util::Version(l.implementationVersion).toString(),
                                   util::Version(l.specVersion).toString()));
        }
        validationLayer = std::make_shared<validation::OMRendererVkValidation>(layers);
        validationLayer->attachExts(&exts);
        return util::OMResult<std::vector<const char *>, std::string>::ok(exts);
    }
    catch (SystemError e)
    {
        logger->error(VkErrorTranslate(e, "openminecraft.renderer.vk.err.preinstance"));
        return util::OMResult<std::vector<const char *>, std::string>::err(
            VkErrorTranslate(e, "openminecraft.renderer.vk.err.preinstance"));
    }
}
void *vkAlloc(void *, size_t size, size_t align, VkSystemAllocationScope s)
{
    void *p = malloc((size / align + 1) * align);
    mem::castorice::rec({mem::castorice::Allocation, p, mem::castorice::heapSize(p), 2});
    return p;
}
void *vkRealloc(void *, void *o, size_t size, size_t align, VkSystemAllocationScope s)
{
    void *p;
    if (o != nullptr)
    {
        mem::castorice::rec({mem::castorice::Free, o, mem::castorice::heapSize(o), 2});
        p = realloc(o, (size / align + 1) * align);
        mem::castorice::rec({mem::castorice::Allocation, p, mem::castorice::heapSize(p), 2});
    }
    else
    {
        p = malloc((size / align + 1) * align);
        mem::castorice::rec({mem::castorice::Allocation, p, mem::castorice::heapSize(p), 2});
    }
    return p;
}
void vkFree(void *, void *p)
{
    if (p == nullptr)
        return;
    mem::castorice::rec({mem::castorice::Free, p, mem::castorice::heapSize(p), 2});
}
void vkInternalAlloc(void *, size_t size, VkInternalAllocationType t, VkSystemAllocationScope s)
{
    mem::castorice::rec({mem::castorice::Allocation, nullptr, size, 3});
}
void vkInternalFree(void *, size_t size, VkInternalAllocationType t, VkSystemAllocationScope s)
{
    mem::castorice::rec({mem::castorice::Free, nullptr, size, 3});
}
OMRendererVk::~OMRendererVk()
{
}
void OMRendererVk::destroy()
{
    validationLayer->ifEnable([&]() { instance.destroyDebugUtilsMessengerEXT(messenger); });
    instance.destroy(allocator);
    SDL_Vulkan_UnloadLibrary();
}
std::string OMRendererVk::driver()
{
    return "";
}
} // namespace openminecraft::renderer::vk
