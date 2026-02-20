#include <SDL3/SDL_error.h>

#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/fontproc/om_font.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <boost/stacktrace/stacktrace.hpp>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <string>

#include <SDL3/SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>

using namespace openminecraft;
using namespace openminecraft::vm;
using namespace openminecraft::binary::hash;
using namespace std::chrono_literals;

namespace openminecraft::boot
{
static void setupI18nEnv()
{
    i18n::res::registerModule("openminecraft-boot");
    i18n::res::registerModule("openminecraft-renderer");
    i18n::res::registerModule("openminecraft-mem");
    i18n::res::pushResourceRoot("/bootassets");
    i18n::res::load();
}

int boot(std::vector<std::string> args)
{
    log::multithread::registerCurrentThreadName("engineMain");
    auto logger = std::make_unique<log::OMLogger>("boot");

    SDL_SetMemoryFunctions(mem::allocator::tracedMallocSDL, mem::allocator::tracedCallocSDL,
                           mem::allocator::tracedReallocSDL, mem::allocator::tracedFreeSDL);
    if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO))
    {
        logger->info("SDL Status: {}", SDL_GetError());
    }

    logger->info("Setting up i18n environment...");
    setupI18nEnv();
    logger->info(i18n::res::translate("openminecraft.boot.arg"));
    for (auto a : args)
    {
        logger->info(a);
    }

    logger->info("hardware / software status");
    logger->info("CPU Name: {}", os::fetchCpuName());
    logger->info("System: {}, version {}", os::fetchSystemName(), os::fetchSystemVersion());
    logger->info("User: {} / {}", os::fetchUsername(), os::fetchLoginUser());
    logger->info("Total memory: {} bytes", os::fetchMemoryTotal());

    std::string comm;
    while (true)
    {
        std::cout << "pixeltower shell > ";
        std::cin >> comm;

        switch (hash_compile_time(comm.c_str()))
        {
        case "vktest"_hash: {
            vulkanRendererTest();
            break;
        }
        case "gltest"_hash: {
            openglRendererTest(); 
            break;
        }
        case "quit"_hash:
        case "exit"_hash:
            goto progEnd;
        case "font"_hash: {
            auto iff = vfs::fsfetch("/bootassets/openminecraft-boot/font/StarRailFont.ttf");
            auto f = new fontproc::OMFont(*iff.get());
            f->buildBasicPolygon(0x2609);
            f->buildBasicPolygon('8');
            delete f;
            break;
        }
        case "dumptrace"_hash:
            logger->dumpStacktrace();
            break;
        case "dumpmem"_hash:
            mem::castorice::printres();
            break;
        case "pt_buildcls"_hash: {
            pixeltowerDynTest();
            break;
        }
        case "ptinit"_hash: {
            pixeltowerLoadTest();
            break;
        }
        case "crash"_hash: {
            logger->info("{}", *reinterpret_cast<int *>(33550336));
        }
        case "png"_hash: {
            auto ist = std::make_shared<std::ifstream>("/home/coder2/output.png", std::ios::binary);
            specs::png::OMPngFile pf(ist);
            logger->info("test!");
            break;
        }
        default:
            logger->warn("unknown command!");
            break;
        }
    }

progEnd:
    SDL_Quit();
    mem::castorice::printres();

    return 0;
}

} // namespace openminecraft::boot
