#include <SDL3/SDL_error.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <boost/stacktrace/stacktrace.hpp>
#include <ctime>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <variant>
#include <vector>

#include "SDL3/SDL_messagebox.h"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_checker.hpp"
#include "openminecraft/vm/bytecode/om_bytecode_descriptor.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/err/om_validation_error.hpp"
#include "openminecraft/vm/heap/om_heap_tree.hpp"
#include <SDL3/SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>

using namespace openminecraft;
using namespace openminecraft::vm::classfile;
using namespace openminecraft::vm::bytecode;

#include "openminecraft/resource/bootassets.h"

namespace openminecraft::boot
{
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

    std::shared_ptr<OMClassFileParser> parser;

    std::string comm;
    while (true)
    {
        std::cout << "> ";
        std::cin >> comm;

        if (comm == "quit" || comm == "exit")
        {
            break;
        }
        else if (comm == "dumptrace")
        {
            logger->dumpStacktrace();
        }
        else if (comm == "dumpmem")
        {
            mem::castorice::printres();
        }
        else if (comm[0] == 'l' && comm[1] == ':')
        {
            parser =
                std::make_shared<OMClassFileParser>(std::make_shared<std::ifstream>(comm.substr(2), std::ios::binary));
            auto clsres = parser->parse();
            switch (clsres.type)
            {
            case Ok: {
                auto clsfile = clsres.unwrap();
                auto chk = std::make_unique<OMBytecodeChecker>(clsfile);
                auto cons = chk->constantCheck();
                switch (cons.type)
                {
                case Ok:
                    break;
                case Err:
                    throw cons.unwrap_err();
                }
                chk->detail();

                break;
            }
            case Err: {
            }
            }
        }
        else
        {
            logger->warn("unknown command");
        }
    }

    mem::castorice::printres();

    return 0;
}
} // namespace openminecraft::boot
