#include <SDL3/SDL_error.h>

#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/io/json/om_io_ast_builder_json.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/specs/abstracts/om_image.hpp"
#include "openminecraft/specs/classfile/om_classfile.hpp"
#include "openminecraft/specs/jfif/om_jfif.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/specs/zip/om_zip.hpp"
#include "openminecraft/specs/zlib/om_zlib_inflaterstream.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <array>
#include <boost/stacktrace/stacktrace.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <SDL3/SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>

using namespace openminecraft;
using namespace openminecraft::io;
using namespace openminecraft::vm;
using namespace openminecraft::binary::hash;

namespace openminecraft::boot
{
static void setupI18nEnv()
{
    i18n::res::registerModule("openminecraft-boot");
    i18n::res::registerModule("openminecraft-renderer");
    i18n::res::pushResourceRoot("/bootassets");
    i18n::res::load();
}

auto boot(std::vector<std::string> args) -> int
{
    log::multithread::registerCurrentThreadName("engineMain");
    auto logger = std::make_unique<log::OMLogger>("boot");

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

    logger->info("hardware / software status");
    logger->info("CPU Name: {}", os::fetchCpuName());
    logger->info("System: {}, version {}", os::fetchSystemName(), os::fetchSystemVersion());
    logger->info("User: {} / {}", os::fetchUsername(), os::fetchLoginUser());
    logger->info("Total memory: {} bytes", os::fetchMemoryTotal());

    switch (hash_compile_time(args[1].c_str()))
    {
    case "vktest"_hash:
        vulkanRendererTest();
        break;
    case "gltest"_hash:
        openglRendererTest();
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
        auto ll = bld.build();

        logger->info("object at {}", (void *)ll.get());
        break;
    }
    case "zip"_hash: {
        specs::zip::OMZip zip;
        zip.parse(std::make_shared<std::ifstream>(args[2], std::ios::binary));
        break;
    }
    case "class"_hash: {
        for (int i = 0; i < 3000; ++i)
        {
            auto ll = new specs::classfile::OMClassFile();
            ll->load(std::make_shared<std::ifstream>(args[2], std::ios::binary));
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
