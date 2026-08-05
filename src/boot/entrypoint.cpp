#include <SDL3/SDL_error.h>

#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/fontproc/om_font.hpp"
#include "openminecraft/fontproc/om_font_outline.hpp"
#include "openminecraft/fontproc/om_fontset.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
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
#include <string>

#include <SDL3/SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace openminecraft;
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
    logger->info("User: {} / {}", os::fetchUsername(), os::fetchLoginUser());
    logger->info("Total memory: {} bytes", os::fetchMemoryTotal());
    args.resize(3);
    args[1] = "3dtest";
    args[2] = "vk";

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
    case "font"_hash: {
        fontproc::OMFontSet fset;
        auto target = args[2];
        for (int i = 3; i < args.size(); ++i)
        {
            std::ifstream istr(args[i], std::ios::binary);
            fset.fontList.push_back(std::make_shared<fontproc::OMFont>(istr));
        }
        auto result = fset.shape(target);
        for (auto &r : result)
        {
            logger->info("#{} 0x{:x} {},{} {},{}", r.fontId, r.glyphId, r.position.x, r.position.y, r.size.x, r.size.y);
        }
        auto bb = fset.bound(target);
        logger->debug("text extent {} {}", bb.x, bb.y);
        break;
    }
    case "angle"_hash: {
        glm::vec3 defaultNormal = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 targetNormal = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
        float selfRotation = glm::radians(22.0f);

        glm::quat q1 = glm::rotation(defaultNormal, targetNormal);
        glm::quat q2 = glm::angleAxis(selfRotation, targetNormal);
        glm::quat q = q2 * q1;
        logger->debug("{}, {}, {}, {}", q.x, q.y, q.z, q.w);
        break;
    }
    case "glyph"_hash: {
        std::ifstream istr(args[2], std::ios::binary);
        fontproc::OMFont f(istr);
        auto test = f.buildOutline(std::stoi(args[3], nullptr, 16), false);
        auto scale = f.scale();
        for (auto &op : test.operations)
        {
            op.target /= scale;
            op.control1 /= scale;
            op.control2 /= scale;
            switch (op.type)
            {
            case fontproc::Move:
                logger->info("Move To {},{}", op.target.x, op.target.y);
                break;
            case fontproc::Line:
                logger->info("Line To {},{}", op.target.x, op.target.y);
                break;
            case fontproc::Quadratic:
                logger->info("Quad To {},{} control {},{}", op.target.x, op.target.y, op.control1.x, op.control1.y);
                break;
            case fontproc::Cubic:
                logger->info("Cubic To {},{} control1 {},{} control2 {},{}", op.target.x, op.target.y, op.control1.x,
                             op.control1.y, op.control2.x, op.control2.y);
                break;
            case fontproc::Close:
                logger->info("Close");
                break;
            }
        }
        auto s = f.fetchBox(std::stoi(args[3], nullptr, 16), false);
        logger->info("{} SVG Operations {} x {}", test.operations.size(), s.y - s.x, s.w - s.z);
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
