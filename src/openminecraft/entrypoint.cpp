#include <SDL3/SDL_error.h>

#include "SDL3/SDL_messagebox.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft-shell/entrypoint.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/io/json/om_io_ast_json.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/renderer/om_renderer_window.hpp"
#include "openminecraft/specs/abstracts/om_image.hpp"
#include "openminecraft/specs/jfif/om_jfif.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include "openminecraft/io/json/om_io_ast_builder_json.hpp"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <boost/stacktrace/stacktrace.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <SDL3/SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace openminecraft;
using namespace openminecraft::io;
using namespace openminecraft::vm;
using namespace openminecraft::binary::hash;

namespace openminecraftshell
{
// INFO: i18n environment initialization
static void setupI18nEnv()
{
    i18n::res::registerModule("openminecraft-boot");
    i18n::res::registerModule("openminecraft-renderer");
    i18n::res::pushResourceRoot("/bootassets");
    i18n::res::load();
}

void prettyJson(std::shared_ptr<json::OMJsonNode> node, std::shared_ptr<log::OMLogger> logger, int l = 0)
{
    switch (node->type())
    {
    case io::json::Object:
        logger->info("{} {{", std::string(l * 4, ' '));
        for (auto &pp : node->getMap())
        {
            logger->info("{} \"{}\" = ", std::string(l * 4, ' '), pp.first);
            prettyJson(pp.second, logger, l + 1);
        }
        logger->info("{} }}", std::string(l * 4, ' '));
        break;
    case io::json::Array:
        logger->info("{} [", std::string(l * 4, ' '));
        for (auto &pp : node->getArray())
        {
            prettyJson(pp, logger, l + 1);
        }
        logger->info("{} ]", std::string(l * 4, ' '));
        break;
    case io::json::Number:
        logger->info("{} {}", std::string(l * 4, ' '), node->getNumberFloating());
        break;
    case io::json::Primitive:
        logger->info("{} {}", std::string(l * 4, ' '), node->getBoolean());
        break;
    case io::json::String:
        logger->info("{} {}", std::string(l * 4, ' '), node->getString());
        break;
    case io::json::Null:
        logger->info("{} null", std::string(l * 4, ' '));
        break;
    }
}

// INFO: main function
auto boot(std::vector<std::string> args) -> int
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
    logger->info("CPU Name: {}", os::fetchCpuName());
    logger->info("System: {}, version {}", os::fetchSystemName(), os::fetchSystemVersion());
    logger->info("Total memory: {} bytes", os::fetchMemoryTotal());

    if (args.size() < 3)
    {
        args.resize(3);
        args[1] = "3dtest";
        args[2] = "gl";
    }

    switch (hash_compile_time(args[1].c_str()))
    {
    case "3dtest"_hash:
        rendererTest(args[2] == "gl" ? renderer::OpenGL : renderer::Vulkan);
        break;
    case "model"_hash: {
        auto ff = vfs::fsfetch(fmt::format("/bootassets/external/minecraft/{}/{}.json", args[2], args[3]));
        json::OMJsonAstBuilder bld(std::make_shared<json::OMJsonTokenIter>(ff));
        auto ll = bld.build();

        prettyJson(ll, logger);
        break;
    }
    default:
        return 1;
    }

progEnd:
    SDL_Quit();

    return 0;
}

} // namespace openminecraftshell
