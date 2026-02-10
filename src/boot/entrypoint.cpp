#include <SDL3/SDL_error.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <any>
#include <boost/stacktrace/stacktrace.hpp>
#include <chrono>
#include <csetjmp>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <stack>
#include <stdexcept>
#include <string>
#include <thread>
#include <typeindex>
#include <variant>
#include <vector>

#include "SDL3/SDL_dialog.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_messagebox.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/fontproc/om_font.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/mem/om_mem_prealloc.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/om_renderer_testrenderer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/specs/vfsbundle/om_vfsbundle.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_checker.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/encoding/om_encoding_utf.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include "openminecraft/vm/pixeltower/internal/om_pixeltower_funcs.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_oop.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_threads.hpp"
#include "openminecraft/vm/pixeltower/v1/om_pixeltower_debugger.hpp"
#include "openminecraft/vm/pixeltower/v1/om_pixeltower_tracing.hpp"
#include "openminecraft/vm/pixeltower/v3/om_pixeltower_classbuilder.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>

using namespace openminecraft;
using namespace openminecraft::vm;
using namespace openminecraft::binary::hash;
using namespace std::chrono_literals;

extern jmp_buf recoverBuffer;

namespace openminecraft::boot
{
std::shared_ptr<pixeltower::v0::OMPixelTower> tower;
log::OMLogger logger("Crash Handler");

void onCrash(int code, int pid, std::vector<openminecraft::vm::pixeltower::v1::tracing::OMTracingFrame> &frames)
{
    logger.debug("tracing stack... (exit code {})", code);
    tower->handleCrash(code, pid, frames);
}

void searchDir(std::vector<std::string> &i, std::filesystem::directory_iterator di)
{
    for (auto entry : di)
    {
        if (entry.is_directory())
        {
            searchDir(i, std::filesystem::directory_iterator(entry));
        }

        if (entry.is_regular_file())
        {
            i.push_back(entry.path().string());
        }
    }
}

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
            try
            {
                renderer::AppInfo a = {"OpenMinecraft", util::Version(1, 0, 0, 0), "OpenMinecraft Engine",
                                       util::Version(1, 0, 0, 0), util::Version(1, 2, 0, 0)};

                auto wnd = SDL_CreateWindow("Vulkan Test", 800, 800, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
                auto renderer = new renderer::vk::OMRendererVk(a, [](std::vector<std::string>) { return 0; }, wnd);

                // SDL_ShowWindow(wnd);

                logger->info("driver: {}", renderer->driver());

                bool wk = false, ak = false, sk = false, dk = false, spk = false, lshk = false;

                while (true)
                {
                    SDL_Event e;
                    SDL_PollEvent(&e);

                    if (e.type == SDL_EVENT_KEY_DOWN)
                    {
                        if (e.key.key == SDLK_W)
                        {
                            wk = true;
                        }
                        else if (e.key.key == SDLK_A)
                        {
                            ak = true;
                        }
                        else if (e.key.key == SDLK_S)
                        {
                            sk = true;
                        }
                        else if (e.key.key == SDLK_D)
                        {
                            dk = true;
                        }
                        else if (e.key.key == SDLK_LSHIFT)
                        {
                            lshk = true;
                        }
                        else if (e.key.key == SDLK_SPACE)
                        {
                            spk = true;
                        }
                        else if (e.key.key == SDLK_ESCAPE)
                        {
                            SDL_SetWindowRelativeMouseMode(wnd, false);
                        }
                    }

                    if (e.type == SDL_EVENT_KEY_UP)
                    {
                        if (e.key.key == SDLK_W)
                        {
                            wk = false;
                        }
                        else if (e.key.key == SDLK_A)
                        {
                            ak = false;
                        }
                        else if (e.key.key == SDLK_S)
                        {
                            sk = false;
                        }
                        else if (e.key.key == SDLK_D)
                        {
                            dk = false;
                        }
                        else if (e.key.key == SDLK_LSHIFT)
                        {
                            lshk = false;
                        }
                        else if (e.key.key == SDLK_SPACE)
                        {
                            spk = false;
                        }
                    }

                    if (e.type == SDL_EVENT_WINDOW_RESIZED)
                    {
                        renderer->needRebuild = true;
                    }

                    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    {
                        SDL_SetWindowRelativeMouseMode(wnd, true);
                    }

                    if (e.type == SDL_EVENT_MOUSE_MOTION && SDL_GetWindowRelativeMouseMode(wnd))
                    {
                        int ww, hh;
                        SDL_GetWindowSize(wnd, &ww, &hh);
                        reinterpret_cast<renderer::test::OMTestRenderer *>(renderer->testRenderer)
                            ->mouseOffset(e.motion.xrel / ww, e.motion.yrel / hh);
                    }

                    if (e.type == SDL_EVENT_QUIT)
                    {
                        renderer->logicalDevice.waitIdle();
                        break;
                    }

                    reinterpret_cast<renderer::test::OMTestRenderer *>(renderer->testRenderer)
                        ->keyInput(wk, ak, sk, dk, lshk, spk);
                    renderer->render();
                }

                delete renderer;
                SDL_DestroyWindow(wnd);

                mem::castorice::printres();
            }
            catch (std::runtime_error &e)
            {
                if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Vulkan Debugger", e.what(), nullptr))
                {
                    logger->info("SDL Status: {}", SDL_GetError());
                }
            }
            break;
        }
        case "gltest"_hash: {
            renderer::AppInfo a = {"OpenMinecraft", util::Version(1, 0, 0, 0), "OpenMinecraft Engine",
                                   util::Version(1, 0, 0, 0), util::Version(3, 3, 0, 0)};

            auto wnd2 = SDL_CreateWindow("OpenGL Test", 800, 800, SDL_WINDOW_OPENGL);
            auto renderer = new renderer::opengl::OMRendererOpenGL(a, wnd2);

            delete renderer;
            SDL_DestroyWindow(wnd2);
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
