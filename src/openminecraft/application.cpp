#include <SDL3/SDL_error.h>

#include "openminecraft-shell/rendererloop.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/renderer/om_renderer_window.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <boost/stacktrace/stacktrace.hpp>
#include <memory>
#include <string>

#include <SDL3/SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>
#include <vector>

#include "openminecraft-shell/application.hpp"

using namespace openminecraft;

namespace openminecraftshell
{
OMApplication::OMApplication(std::vector<std::string> args) : logger("OMApplication", this), args(args)
{
}
OMApplication::~OMApplication() = default;

static void setupI18nEnv()
{
    i18n::res::registerModule("openminecraft-boot");
    i18n::res::registerModule("openminecraft-renderer");
    i18n::res::pushResourceRoot("/bootassets");
    i18n::res::load();
}

auto OMApplication::entry() -> int
{
    log::multithread::registerCurrentThreadName("engineMain");
    auto logger = std::make_shared<log::OMLogger>("boot");

    SDL_SetMemoryFunctions(mem::allocator::tracedMallocSDL, mem::allocator::tracedCallocSDL,
                           mem::allocator::tracedReallocSDL, mem::allocator::tracedFreeSDL);
    setupI18nEnv();
    if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO))
    {
        logger->info("SDL Status: {}", SDL_GetError());
    }
    logger->info(i18n::res::translate("openminecraft.boot.arg"));
    for (auto a : args)
    {
        logger->info(a);
    }

    // INFO: hardware information
    logger->info("hardware / software status");
    logger->info("CPU Name: {}", openminecraft::vm::os::fetchCpuName());
    logger->info("System: {}, version {}", openminecraft::vm::os::fetchSystemName(),
                 openminecraft::vm::os::fetchSystemVersion());
    logger->info("Total memory: {} bytes", openminecraft::vm::os::fetchMemoryTotal());

    renderer::OMBackend bk = renderer::Vulkan;
    if (args.size() >= 2)
    {
        bk = args[1] == "gl" ? renderer::OpenGL : renderer::Vulkan;
    }

    rendererLoop(bk);

    SDL_Quit();

    return 0;
}
} // namespace openminecraftshell