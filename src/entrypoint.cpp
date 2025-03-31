#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include "vulkan/vulkan.hpp"
#include <iostream>

// Vulkan defined macro storage for dispatch loader
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

int main()
{
    vk::DynamicLoader dl("libvulkan.so");
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    vk::Instance instance = vk::createInstance({}, nullptr);
    // initialize function pointers for instance
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    // create a dispatcher, based on additional vkDevice/vkGetDeviceProcAddr
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();  
    std::cout << physicalDevices.size();
    vk::Device device = physicalDevices[0].createDevice({}, nullptr);
    // function pointer specialization for device
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
}