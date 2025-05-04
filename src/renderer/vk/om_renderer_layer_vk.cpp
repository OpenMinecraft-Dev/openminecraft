#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include "vulkan/vulkan_core.h"
#include <SDL3/SDL_vulkan.h>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#ifdef OM_VULKAN_DYNAMIC
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

using namespace vk;
using openminecraft::i18n::res::translate;
namespace openminecraft::renderer::vk
{
OMRendererVk::OMRendererVk(AppInfo info, std::function<int(std::vector<std::string>)> dev) : OMRenderer(info)
{
    logger = std::make_shared<log::OMLogger>("OMRendererVk", this);

    allocator = {nullptr,
                 (PFN_AllocationFunction)vkAlloc,
                 (PFN_ReallocationFunction)vkRealloc,
                 (PFN_FreeFunction)vkFree,
                 (PFN_InternalAllocationNotification)vkInternalAlloc,
                 (PFN_InternalFreeNotification)vkInternalFree};

    // Dynamic loader init
    {
#ifdef OM_VULKAN_DYNAMIC
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
            loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
#endif
    }

    // Required extensions
    unsigned int extcount = 0;
    const char *const *ext;
    {
        ext = SDL_Vulkan_GetInstanceExtensions(&extcount);

        auto layers = enumerateInstanceLayerProperties();
        logger->info(translate("openminecraft.renderer.vk.layercount", layers.size()));
        for (auto l : layers)
        {
            logger->info(translate("openminecraft.renderer.vk.layerdata", l.layerName.data(), l.layerName.data(),
                                   util::Version(l.implementationVersion).toString(),
                                   util::Version(l.specVersion).toString()));
        }
    }

    // Instance
    {
        ApplicationInfo i(info.appName.c_str(), info.appVer.toVKVersion(), info.engineName.c_str(),
                          info.engineVer.toVKVersion(), info.minApiVersion.toVKApiVersion());
        InstanceCreateInfo ii(InstanceCreateFlags(), &i, 0, nullptr, extcount, ext);
        instance = createInstance(ii, allocator);
        logger->info(translate("openminecraft.renderer.vk.instance", info.appName, info.appVer.toString(),
                               info.engineName, info.engineVer.toString(), info.minApiVersion.toString()));
#ifdef OM_VULKAN_DYNAMIC
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
#endif
    }

    auto phyDev = instance.enumeratePhysicalDevices();
    logger->info(translate("openminecraft.renderer.vk.devcount", phyDev.size()));
    int id = 0;
    for (auto pdev : phyDev)
    {
        logger->info(pdev.getProperties().deviceName);
        id++;
    }
    /*std::vector<PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    logger->info(translate("openminecraft.renderer.vk.devcount", physicalDevices.size()));
    Device device = physicalDevices[0].createDevice({}, allocator);
#ifdef OM_VULKAN_DYNAMIC
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
#endif
    device.destroy(allocator);*/
}
void *vkAlloc(void *, size_t size, size_t align, VkSystemAllocationScope s)
{
    void *p = malloc((size / align + 1) * align);
    mem::castorice::rec({mem::castorice::Allocation, p, (size / align + 1) * align, 2});
    return p;
}
void *vkRealloc(void *, void *o, size_t size, size_t align, VkSystemAllocationScope s)
{
    void *p;
    if (o != nullptr)
    {
        mem::castorice::rec({mem::castorice::Free, o, mem::castorice::heapSize(o), 2});
        p = realloc(o, (size / align + 1) * align);
        mem::castorice::rec({mem::castorice::Allocation, p, (size / align + 1) * align, 2});
    }
    else
    {
        p = malloc((size / align + 1) * align);
        mem::castorice::rec({mem::castorice::Allocation, p, (size / align + 1) * align, 2});
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
    instance.destroy(allocator);
}
std::string OMRendererVk::driver()
{
    return "";
}
} // namespace openminecraft::renderer::vk