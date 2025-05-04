#include <SDL3/SDL_error.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <boost/stacktrace/stacktrace.hpp>
#include <memory>
#include <vector>

#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <SDL3/SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>

using namespace openminecraft;

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

    /*if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Hello World",
                                  "!! Your SDL project successfully runs on Android!!", NULL))
    {
        logger->info("SDL Status: {}", SDL_GetError());
        return 1;
    }*/

    logger->info("Setting up i18n environment...");
    i18n::res::registerModule("openminecraft-boot");
    i18n::res::registerModule("openminecraft-renderer");
    vfs::fsmountBundle({res_bundle, res_bundle_len}, "/bootassets");
    i18n::res::pushResourceRoot("/bootassets");
    i18n::res::load();

    logger->info(i18n::res::translate("openminecraft.boot.arg"));
    for (auto a : args)
    {
        logger->info(a);
    }

    renderer::AppInfo a = {"OpenMinecraft", util::Version(1, 0, 0, 0), "OpenMinecraft Engine",
                           util::Version(1, 0, 0, 0), util::Version(1, 0, 0, 0)};
    auto renderer = std::make_unique<renderer::vk::OMRendererVk>(a);

    vfs::fsumount("/bootassets");

    SDL_Quit();

    return 0;
}
} // namespace openminecraft::boot
