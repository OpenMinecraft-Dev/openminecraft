#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <vector>
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"
#include "shaderc/shaderc.hpp"
#include <sstream>

#ifdef OM_VULKAN_DYNAMIC
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a ## b)
#define PP_CAT_II(p, res) res
#define NM(base) PP_CAT(base, __LINE__)
#define LOG_TEST(f, caller) std::stringstream NM(temp);NM(temp) << f; caller(NM(temp).str())

using namespace openminecraft::log;
using namespace openminecraft::log::multithraad;

int main()
{
    registerCurrentThreadName("engineMain");
    auto logger = new OMLogger("test");

#ifdef OM_VULKAN_DYNAMIC
    vk::DynamicLoader dl;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
#endif
    vk::Instance instance = vk::createInstance({}, nullptr);
#ifdef OM_VULKAN_DYNAMIC
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
#endif
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    std::stringstream s1; s1 << "Vulkan devices: " << physicalDevices.size(); logger->info(s1.str());
    vk::Device device = physicalDevices[0].createDevice({}, nullptr);
#ifdef OM_VULKAN_DYNAMIC
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
#endif
    shaderc::Compiler comp;
    LOG_TEST("Shaderc available: " << comp.IsValid(), logger->info);
    // std::stringstream s2; s2 << "Shaderc available: " << comp.IsValid(); logger->info(s2.str());
    
    logger->debug("test!");
    logger->info("test!");
    LOG_TEST("hello *OMLogger = " << logger << "!", logger->info);
    // std::stringstream s; s << "hellp *OMLogger = " << logger << "!"; logger->info(s.str());
    logger->warn("test!");
    logger->error("test!");
    logger->fatal("test!");
}
