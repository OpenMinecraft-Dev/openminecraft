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
#include "openminecraft/vm/pixeltower/om_pixeltower.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_array_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_debugger.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_frame_structdef.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_interpreter.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_memorymanager.hpp"
#include <SDL3/SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>

using namespace openminecraft;
using namespace openminecraft::vm::classfile;
using namespace openminecraft::vm::bytecode;
using namespace openminecraft::binary::hash;
using namespace openminecraft::vm::pixeltower;

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

    auto pt = std::make_unique<OMPixelTower>();
    auto deb =
        std::make_unique<v1::OMDebugger>(std::any_cast<std::shared_ptr<runtime::OMInterpreter>>(pt->interpreter));

    std::shared_ptr<OMClassFileParser> parser;
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
                if (commandBuffer.size() > 2)
                {
                    if (commandBuffer[1] == "loadcls")
                    {
                        try
                        {
                            pt->loadClass(std::make_shared<std::ifstream>(commandBuffer[2], std::ios::binary));

                            auto cls = pt->fetchClass("java/lang/String");
                            void *arr = pt->allocateArray(cls, 2);
                            void *str = std::any_cast<std::shared_ptr<runtime::OMInterpreter>>(pt->interpreter)
                                            ->newString("--debug");
                            void *str2 = std::any_cast<std::shared_ptr<runtime::OMInterpreter>>(pt->interpreter)
                                             ->newString("test");

                            ARRAY_ACCESS(arr, void *)[0] = str;
                            ARRAY_ACCESS(arr, void *)[1] = str2;
                            std::any_cast<std::shared_ptr<runtime::OMInterpreter>>(pt->interpreter)
                                ->stack[std::this_thread::get_id()]
                                .push(arr);

                            pt->execute("openminecraft/Test", "main", "([Ljava/lang/String;)V");
                        }
                        catch (vm::err::OMValidationError err)
                        {
                            logger->error(err.what());
                            deb->printStack();
                        }

                        commandBuffer.clear();
                    }
                    else if (commandBuffer[1] == "memtest")
                    {
                        std::vector<void *> pp;
                        try
                        {
                            while (true)
                            {
                                pp.push_back(pt->mm->allocate(pt->fetchClass("java/lang/String")));
                            }
                        }
                        catch (std::logic_error e)
                        {
                            pt->mm->debug();
                        }

                        pt->mm->deallocate(pp[0]);
                        pt->mm->deallocate(pp[2]);
                        pt->mm->deallocate(pp[1]);
                        pt->mm->debug();

                        commandBuffer.clear();
                    }
                    else if (commandBuffer[1] == "init")
                    {
                        std::vector<std::string> m;
                        searchDir(m, std::filesystem::directory_iterator(commandBuffer[2]));
                        for (auto a : m)
                        {
                            pt->loadClass(std::make_shared<std::ifstream>(a, std::ios::binary));
                        }

                        auto cmdPrint = [&](std::any data) {
                            /*logger->debug("[stdout] {}",
                                          pt->printAny(*(++std::any_cast<std::list<std::any> *>(data)->begin()), 0));*/
                            logger->debug("[stdout] not implemented");
                            return nullptr;
                        };

                        pt->registerNativeFunc("vmstd/internal/SystemPrintStream", "println", "(C)V", cmdPrint);
                        pt->registerNativeFunc("vmstd/internal/SystemPrintStream", "println", "(J)V", cmdPrint);
                        pt->registerNativeFunc("vmstd/internal/SystemPrintStream", "println", "(F)V", cmdPrint);
                        pt->registerNativeFunc("vmstd/internal/SystemPrintStream", "println", "(D)V", cmdPrint);
                        pt->registerNativeFunc("vmstd/internal/SystemPrintStream", "println", "(I)V", cmdPrint);
                        pt->registerNativeFunc("vmstd/internal/SystemPrintStream", "println", "(Z)V", cmdPrint);
                        pt->registerNativeFunc("vmstd/internal/SystemPrintStream", "println", "(Ljava/lang/Object;)V",
                                               [&](std::any s) {
                                                   auto argl = std::any_cast<std::list<std::any> *>(s);
                                                   // logger->debug("[stdout] {}", std::any_cast<void
                                                   // *>(*(++argl->begin())));
                                                   return nullptr;
                                               });
                        pt->registerNativeFunc(
                            "vmstd/internal/SystemPrintStream", "println", "(Ljava/lang/String;)V", [&](std::any s) {
                                auto argl = std::any_cast<std::list<std::any> *>(s);
                                auto item = std::any_cast<void *>(*(++argl->begin()));
                                auto arr = (OMArrayHeader *)*(void **)OBJECT_ACCESS(item, sizeof(void *));
                                auto str = std::string(ARRAY_ACCESS(arr, char), arr->length);
                                logger->debug("[stdout] {}", str);
                                return nullptr;
                            });

                        commandBuffer.clear();
                    }
                }
                break;
            }
            case "crash"_hash: {
                logger->info("{}", *((int *)nullptr));
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
