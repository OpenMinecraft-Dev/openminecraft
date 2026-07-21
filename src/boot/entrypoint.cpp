#include <SDL3/SDL_error.h>

#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/io/json/om_io_ast_builder_json.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/om_renderer_window.hpp"
#include "openminecraft/specs/abstracts/om_image.hpp"
#include "openminecraft/specs/jfif/om_jfif.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/util/om_util_ticker.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <boost/stacktrace/stacktrace.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>

#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"

#include <SDL3/SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>
#include <vector>

using namespace openminecraft;
using namespace openminecraft::io;
using namespace openminecraft::vm;
using namespace openminecraft::binary::hash;

namespace openminecraft::boot
{
// INFO: i18n environment initialization
static void setupI18nEnv()
{
    i18n::res::registerModule("openminecraft-boot");
    i18n::res::registerModule("openminecraft-renderer");
    i18n::res::pushResourceRoot("/bootassets");
    i18n::res::load();
}

// INFO: main function
auto boot(std::vector<std::string> args) -> int
{
    log::multithread::registerCurrentThreadName("engineMain");
    auto logger = std::make_unique<log::OMLogger>("boot");

    util::OMTicker ticker;
    ticker.begin();

    ticker.push("base_init");

    ticker.push("sdl_mem");
    SDL_SetMemoryFunctions(mem::allocator::tracedMallocSDL, mem::allocator::tracedCallocSDL,
                           mem::allocator::tracedReallocSDL, mem::allocator::tracedFreeSDL);
    ticker.pop();

    ticker.push("i18n");
    setupI18nEnv();
    ticker.pop();

    ticker.push("sdl_main");
    if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO))
    {
        logger->info("SDL Status: {}", SDL_GetError());
    }
    ticker.pop();

    ticker.pop();

    for (auto &event : ticker.ticks)
    {
        logger->debug("{}{} -> {} ns", event.first.pop ? "-" : "+", event.first.id, event.second);
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
    logger->info("User: {} / {}", os::fetchUsername(), os::fetchLoginUser());
    logger->info("Total memory: {} bytes", os::fetchMemoryTotal());

    switch (hash_compile_time(args[1].c_str()))
    {
    case "3dtest"_hash:
        rendererTest(args[2] == "gl" ? renderer::OpenGL : renderer::Vulkan);
        break;
    case "pic"_hash: {
        auto in = args[2];
        auto out = args[3];

        specs::OMImage *img;

        if (in.find(".png") != -1)
        {
            img = new specs::png::OMPngFile();
        }
        else if (in.find(".jpg") != -1 || in.find(".jpeg") != -1)
        {
            img = new specs::jfif::OMJfifFile();
        }

        img->parseBase(std::make_shared<std::ifstream>(in, std::ios::binary));
        std::ofstream of(out, std::ios::binary);
        of.write(reinterpret_cast<char *>(img->fetchData()), img->getWidth() * img->getHeight() * 4);
        of.close();

        delete img;

        break;
    }
    case "json"_hash: {
        auto ii = std::make_shared<std::istringstream>(args[2]);
        json::OMJsonAstBuilder bld(std::make_shared<json::OMJsonTokenIter>(ii));
        logger->info(args[2]);
        auto obj = bld.build();

        logger->info("object at {}", reinterpret_cast<void *>(obj.get()));
        break;
    }
    case "layout"_hash: {
        std::random_device rd;
        std::default_random_engine eng(rd());
        std::uniform_real_distribution<float> distr(40.0, 100.0);

        using namespace openminecraft::renderer::common::demiurge;
        auto root = std::make_shared<OMDemiurgeNode>()->flexDirection(Row)->flexWrap(Wrap);
        std::vector<std::shared_ptr<OMDemiurgeNode>> chds;
        for (int i = 0; i < 150; ++i)
        {
            auto chd1 = std::make_shared<OMDemiurgeNode>()
                            ->width(OMDemiurgeSize::percent(0.25))
                            ->height(OMDemiurgeSize::pixels(distr(eng)))
                            ->border({10, 10, 10, 10});
            root->mount(chd1);
            chds.push_back(chd1);
        }
        root->layout(800, 600);

        for (auto n : chds)
        {
            auto ll = n->boundary();
            logger->warn("{} {} {} {}", ll.x, ll.y, ll.width, ll.height);
        }
        break;
    }
    default:
        return 1;
    }

progEnd:
    SDL_Quit();

    return 0;
}

} // namespace openminecraft::boot
