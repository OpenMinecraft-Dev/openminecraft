#include <SDL3/SDL_error.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <any>
#include <boost/stacktrace/stacktrace.hpp>
#include <chrono>
#include <csetjmp>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <new>
#include <stack>
#include <stdexcept>
#include <string>
#include <thread>
#include <typeindex>
#include <variant>
#include <vector>

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
#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include <SDL3/SDL.h>
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

    /*try
    {
        renderer::AppInfo a = {"OpenMinecraft", util::Version(1, 0, 0, 0), "OpenMinecraft Engine",
                               util::Version(1, 0, 0, 0), util::Version(1, 0, 0, 0)};
        auto renderer = std::make_unique<renderer::vk::OMRendererVk>(a, [](std::vector<std::string>) { return 0; });

        mem::castorice::printres();
        vfs::fsumount("/bootassets");

        renderer->destroy();
    }
    catch (std::runtime_error e)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Vulkan Debugger", e.what(), NULL))
        {
            logger->info("SDL Status: {}", SDL_GetError());
            return 1;
        }
    }*/

    SDL_Quit();

    auto tower = std::make_unique<pixeltower::v0::OMPixelTower>();

    bool recovermode = false;

    if (setjmp(recoverBuffer) == 0)
    {
    program:
        std::string comm;
        std::vector<std::string> commandBuffer;
        while (true)
        {
            if (commandBuffer.empty())
            {
                std::cout << (recovermode ? "pixeltower recover > " : "pixeltower shell > ");
            }

        unk:
            std::cin >> comm;
            commandBuffer.push_back(comm);

            switch (hash_compile_time(commandBuffer[0].c_str()))
            {
            case "quit"_hash:
            case "exit"_hash:
                commandBuffer.clear();
                return 0;
            case "dumptrace"_hash:
                commandBuffer.clear();
                logger->dumpStacktrace();
                break;
            case "dumpmem"_hash:
                commandBuffer.clear();
                mem::castorice::printres();
                break;
            case "pixeltower"_hash: {
                if (commandBuffer.size() >= 3 && commandBuffer[1] == "init")
                {
                    tower->init(commandBuffer[2]);
                    tower->loader->loadClass("vmstd/internal/SystemPrintStream");
                    logger->info("{}", (void *)tower->loader->fetchClass("vmstd/internal/SystemPrintStream"));
                    commandBuffer.clear();
                    break;
                }
                goto unk;
            }
            case "crash"_hash: {
                logger->info("{}", *((int *)33550336));
            }
            default:
                commandBuffer.clear();
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

    return 0;
}

} // namespace openminecraft::boot
