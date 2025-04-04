#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_plat.hpp"
#include "openminecraft/log/om_log_ansi.hpp"
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"
#include <iostream>
#include "shaderc/shaderc.hpp"

#ifdef OM_VULKAN_DYNAMIC
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

using namespace openminecraft::log;

int main()
{
#ifdef OM_VULKAN_DYNAMIC
    vk::DynamicLoader dl;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
#endif
    vk::Instance instance = vk::createInstance({}, nullptr);
    // initialize function pointers for instance
#ifdef OM_VULKAN_DYNAMIC
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
#endif
    // create a dispatcher, based on additional vkDevice/vkGetDeviceProcAddr
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();  
    std::cout << "Vulkan devices: " << physicalDevices.size() << std::endl;
    vk::Device device = physicalDevices[0].createDevice({}, nullptr);
    // function pointer specialization for device
#ifdef OM_VULKAN_DYNAMIC
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
#endif
    shaderc::Compiler comp;
    std::cout << (comp.IsValid() ? "true" : "false") << std::endl;

    auto logger = new OMLogger("test", getPlatformLoggingStream());
    logger->info(std::string(OMLogAnsiRed).append("test!"));
}