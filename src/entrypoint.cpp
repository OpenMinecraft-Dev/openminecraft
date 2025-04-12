#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/vm/om_class_file.hpp"
#include <SDL3/SDL_error.h>
#include <fstream>
#include <stdexcept>
#include <vector>
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"
#include "shaderc/shaderc.hpp"
#include <SDL3/SDL.h>
#include <fmt/format.h>

#ifdef OM_VULKAN_DYNAMIC
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

using namespace openminecraft::log;
using namespace openminecraft::log::multithraad;
using namespace openminecraft::vm::classfile;

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
    logger->info("Vulkan devices: {}", physicalDevices.size());
    vk::Device device = physicalDevices[0].createDevice({}, nullptr);
#ifdef OM_VULKAN_DYNAMIC
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
#endif
    shaderc::Compiler comp;
    logger->info("Shaderc available: {}", comp.IsValid());
    logger->info("hello *OMLogger = {}!", fmt::ptr(logger));

    if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
        // SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed (%s)", SDL_GetError());
        return 1;
    }

    /*if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Hello World",
                                 "!! Your SDL project successfully runs on Android !!", NULL)) {
        // SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_ShowSimpleMessageBox failed (%s)", SDL_GetError());
        return 1;
    }*/

    SDL_Quit();

    std::ifstream f("/home/coder2/Test.class", std::ios::binary);
    auto par = new OMClassFileParser(f);
    auto clsfile = par->parse();

    uint32_t cid = 1;
    for (auto c : clsfile->constants)
    {
        switch (c->type())
        {
            case OMClassConstantType::Utf8:
            {
                logger->info("#{} Utf8(\"{}\")", cid, c->to<OMClassConstantUtf8>()->data);
                break;
            }
            default:
            {
                // throw std::invalid_argument(fmt::format("Unknown constant id {}!", (int) c->type()));
            }
        }
        cid++;
    }

    return 0;
}