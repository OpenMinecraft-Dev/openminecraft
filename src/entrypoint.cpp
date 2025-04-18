#include <SDL3/SDL_error.h>

#include <boost/stacktrace/stacktrace.hpp>
#include <fstream>
#include <vector>

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/vm/om_class_file.hpp"
#include "vulkan/vulkan_core.h"
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include <SDL3/SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>
#include <iostream>

#include "shaderc/shaderc.hpp"
#include "vulkan/vulkan.hpp"

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
    device.destroy(nullptr);
    instance.destroy(nullptr);

    shaderc::Compiler comp;
    logger->info("Shaderc available: {}", comp.IsValid());
    logger->info("hello *OMLogger = {}!", fmt::ptr(logger));

    if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
        logger->info("SDL Status: {}", SDL_GetError());
        // SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed (%s)",
        // SDL_GetError());
    }

    if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Hello World", "!! Your SDL project successfully runs on Android!!", NULL)) {
        logger->info("SDL Status: {}", SDL_GetError());
        return 1;
    }

    SDL_Quit();

    std::ifstream f("/home/coder2/Test.class", std::ios::binary);
    auto par = new OMClassFileParser(f);
    auto clsfile = par->parse();

    uint32_t cid = 1;
    for (auto c : clsfile->constants) {
        switch (c->type()) {
        case OMClassConstantType::Utf8: {
            logger->info("#{} Utf8(\"{}\")", cid, c->to<OMClassConstantUtf8>()->data);
            break;
        }
        case OMClassConstantType::Integer: {
            logger->info("#{} Integer({})", cid, c->to<OMClassConstantInteger>()->data);
            break;
        }
        case OMClassConstantType::Float: {
            logger->info("#{} Float({})", cid, c->to<OMClassConstantFloat>()->data);
            break;
        }
        case OMClassConstantType::Long: {
            logger->info("#{} Long({})", cid, c->to<OMClassConstantLong>()->data);
            cid++;
            break;
        }
        case OMClassConstantType::Double: {
            logger->info("#{} Double({})", cid, c->to<OMClassConstantDouble>()->data);
            cid++;
            break;
        }
        case OMClassConstantType::Class: {
            logger->info("#{} Class(#{})", cid, c->to<OMClassConstantClass>()->nameIndex);
            break;
        }
        case OMClassConstantType::String: {
            logger->info("#{} String(#{})", cid, c->to<OMClassConstantString>()->stringIndex);
            break;
        }
        case OMClassConstantType::FieldRef: {
            auto d = c->to<OMClassConstantFieldRef>();
            logger->info("#{} FieldRef(#{}, #{})", cid, d->classIndex, d->nameAndTypeIndex);
            break;
        }
        case OMClassConstantType::MethodRef: {
            auto d = c->to<OMClassConstantMethodRef>();
            logger->info("#{} MethodRef(#{}, #{})", cid, d->classIndex, d->nameAndTypeIndex);
            break;
        }
        case OMClassConstantType::InterfaceMethodRef: {
            auto d = c->to<OMClassConstantInterfaceMethodRef>();
            logger->info("#{} InterfaceMethodRef(#{}, #{})", cid, d->classIndex, d->nameAndTypeIndex);
            break;
        }
        case OMClassConstantType::NameAndType: {
            auto d = c->to<OMClassConstantNameAndType>();
            logger->info("#{} NameAndType(#{}, #{})", cid, d->nameIndex, d->descIndex);
            break;
        }
        case OMClassConstantType::MethodHandle: {
            auto d = c->to<OMClassConstantMethodHandle>();
            logger->info("#{} MethodHandle({}, #{})", cid, (int)d->refKind, d->refIndex);
            break;
        }
        case OMClassConstantType::MethodType: {
            logger->info("#{} MethodType(#{})", cid, c->to<OMClassConstantMethodType>()->descIndex);
            break;
        }
        case OMClassConstantType::Dynamic: {
            auto d = c->to<OMClassConstantDynamic>();
            logger->info("#{} Dynamic(#{}, #{})", cid, d->bootstrapMethodAttrIndex, d->nameAndTypeIndex);
            break;
        }
        case OMClassConstantType::InvokeDynamic: {
            auto d = c->to<OMClassConstantInvokeDynamic>();
            logger->info("#{} InvokeDynamic(#{}, #{})", cid, d->bootstrapMethodAttrIndex, d->nameAndTypeIndex);
            break;
        }
        case OMClassConstantType::Module: {
            logger->info("#{} Module(#{})", cid, c->to<OMClassConstantModule>()->nameIndex);
            break;
        }
        case OMClassConstantType::Package: {
            logger->info("#{} Package(#{})", cid, c->to<OMClassConstantPackage>()->nameIndex);
            break;
        }
        default: {
            throw std::invalid_argument(fmt::format("Unknown constant id {}!", (int)c->type()));
        }
        }
        cid++;
    }

    auto st = boost::stacktrace::stacktrace();
    int i = 0;
    for (auto frame : st) {
        logger->info("#{} 0x{} {} {}:{}", i, frame.address(), frame.name(), frame.source_file(), frame.source_line());
        i++;
    }

    delete par;
    delete clsfile;
    delete logger;

    return 0;
}
