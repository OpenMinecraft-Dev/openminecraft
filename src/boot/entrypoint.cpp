#include <SDL3/SDL_error.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <any>
#include <boost/stacktrace/stacktrace.hpp>
#include <chrono>
#include <csetjmp>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <sstream>
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
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/mem/om_mem_prealloc.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
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

#include "openminecraft/resource/bootassets.h"

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
int boot(std::vector<std::string> args)
{
    std::cin.tie(0);
    std::cout.tie(0);
    log::multithread::registerCurrentThreadName("engineMain");
    auto logger = std::make_unique<log::OMLogger>("boot");

    SDL_SetMemoryFunctions(mem::allocator::tracedMallocSDL, mem::allocator::tracedCallocSDL,
                           mem::allocator::tracedReallocSDL, mem::allocator::tracedFreeSDL);
    if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO))
    {
        logger->info("SDL Status: {}", SDL_GetError());
    }

    logger->info("Setting up i18n environment...");
    i18n::res::registerModule("openminecraft-boot");
    i18n::res::registerModule("openminecraft-renderer");
    i18n::res::registerModule("openminecraft-mem");
    vfs::fsmountBundle({res_bundle, res_bundle_len}, "/bootassets");
    i18n::res::pushResourceRoot("/bootassets");
    i18n::res::load();

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

    /*SDL_ShowFileDialogWithProperties(
        SDL_FileDialogType::SDL_FILEDIALOG_OPENFILE,
        [](void *userdata, const char *const *filelist, int filter) { SDL_Log("testf: %s", filelist[0]); }, nullptr,
        -1);*/

    // pixeltower::registerFuncs();
    tower = std::make_unique<pixeltower::v0::OMPixelTower>();

    bool recovermode = false;

    if (setjmp(recoverBuffer) == 0)
    {
    program:
        std::string comm;
        while (true)
        {
            std::cout << (recovermode ? "pixeltower recover > " : "pixeltower shell > ");

        unk:
            std::cin >> comm;

            switch (hash_compile_time(comm.c_str()))
            {
            case "vktest"_hash: {
                try
                {
                    renderer::AppInfo a = {"OpenMinecraft", util::Version(1, 0, 0, 0), "OpenMinecraft Engine",
                                           util::Version(1, 0, 0, 0), util::Version(1, 0, 0, 0)};

                    auto wnd = SDL_CreateWindow("Vulkan Test", 800, 800, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
                    auto renderer = std::make_unique<renderer::vk::OMRendererVk>(
                        a, [](std::vector<std::string>) { return 0; }, wnd);
                    SDL_ShowWindow(wnd);

                    logger->info("driver: {}", renderer->driver());

                    bool wk = false, ak = false, sk = false, dk = false, spk = false, lshk = false;
                    bool up = false, down = false, left = false, right = false;

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
                            else if (e.key.key == SDLK_UP)
                            {
                                up = true;
                            }
                            else if (e.key.key == SDLK_DOWN)
                            {
                                down = true;
                            }
                            else if (e.key.key == SDLK_LEFT)
                            {
                                left = true;
                            }
                            else if (e.key.key == SDLK_RIGHT)
                            {
                                right = true;
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
                            else if (e.key.key == SDLK_UP)
                            {
                                up = false;
                            }
                            else if (e.key.key == SDLK_DOWN)
                            {
                                down = false;
                            }
                            else if (e.key.key == SDLK_LEFT)
                            {
                                left = false;
                            }
                            else if (e.key.key == SDLK_RIGHT)
                            {
                                right = false;
                            }
                        }

                        if (e.type == SDL_EVENT_WINDOW_RESIZED)
                        {
                            renderer->needRebuild = true;
                        }

                        if (e.type == SDL_EVENT_QUIT)
                        {
                            renderer->logicalDevice.waitIdle();
                            break;
                        }

                        renderer->testRenderer->keyInput(wk, ak, sk, dk, lshk, spk, up, down, left, right);
                        renderer->render();
                    }

                    mem::castorice::printres();

                    renderer->destroy();
                    mem::castorice::printres();

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
            case "quit"_hash:
            case "exit"_hash:
                return 0;
            case "dumptrace"_hash:
                logger->dumpStacktrace();
                break;
            case "dumpmem"_hash:
                mem::castorice::printres();
                break;
            case "pt_buildcls"_hash: {
                pixeltower::v3::OMClassBuilder builder;
                builder.klassBegin();
                builder.klassAccessFlags(JVM_Acc_Public);
                builder.klassName("openminecraft/DynamicTest");

                pixeltower::v1::tracing::installHandler();
                tower->initCurrentThread(1ul * 1024 * 1024);
                tower->init("vmstd/out");

                bytecode::descriptor::OMTypeDesc tgt = {bytecode::descriptor::Reference, "java/lang/Object"};
                tower->loader->loadClass(tgt);
                auto cls = tower->loader->fetchClass(tgt);

                builder.klassSuperKlass(cls);
                builder.klassVersion(JVM_VERSION_8, 0);

                auto func = builder.klassConstructMethod();
                func->methodBegin();
                func->methodAccessFlags(JVM_Acc_Public);
                func->methodNameAndDesc("<init>", "()V");
                func->methodCodeBegin();
                func->instNop();
                func->instLoad<void *>(0);
                func->instConst(421.f);
                func->instReturn();
                func->methodCodeFinish();
                func->methodFinish();

                bytecode::OMBytecodeChecker chk(builder.file);
                chk.detail();

                tower->loader->stagClass(builder.file);
                tower->loader->loadClass({bytecode::descriptor::Reference, "openminecraft/DynamicTest"});

                break;
            }
            case "ptinit"_hash: {
                pixeltower::v1::tracing::installHandler();
                tower->initCurrentThread(1ul * 1024 * 1024);
                tower->init("vmstd/out");
                tower->load("../Test.class");

                bytecode::descriptor::OMTypeDesc tgt = {bytecode::descriptor::Reference, "openminecraft/Test"};
                tower->loader->loadClass(tgt);
                auto cls = tower->loader->fetchClass(tgt);
                auto met = cls->methods;
                while (met != nullptr)
                {
                    if (strcmp(met->name, "main") == 0 && strcmp(met->desc, "([Ljava/lang/String;)V") == 0)
                    {
                        auto now = std::chrono::system_clock::now();

                        bool running = true;
                        auto t = new std::thread([&]() {
                            while (running)
                            {
                                auto now2 = std::chrono::system_clock::now();
                                auto cont = std::chrono::duration_cast<std::chrono::milliseconds>(now2 - now).count();
                                if (cont > 1000)
                                {
                                    logger->info("{} operands in 1 second",
                                                 (double)tower->interpreter->operands / cont * 1000);
                                    tower->interpreter->operands = 0;
                                    now = std::chrono::system_clock::now();
                                }
                            }
                        });

                        try
                        {
                            tower->boot(met);
                        }
                        catch (err::OMValidationError &e)
                        {
                            logger->info("{}", e.what());
                        }
                        catch (int g)
                        {
                        }

                        running = false;
                        auto now2 = std::chrono::system_clock::now();
                        logger->info("VM exited {} ns",
                                     std::chrono::duration_cast<std::chrono::nanoseconds>(now2 - now).count());

                        break;
                    }
                    met = met->next;
                }
                tower->destroyCurrentThread();
            }
            case "crash"_hash: {
                logger->info("{}", *reinterpret_cast<int *>(33550336));
            }
            default:
                logger->warn("unknown command!");
                break;
            }
        }
    }
    else
    {
        logger->info("recovering from sigsegv!");
        recovermode = true;
        goto program;
    }

    SDL_Quit();

    return 0;
}

} // namespace openminecraft::boot
