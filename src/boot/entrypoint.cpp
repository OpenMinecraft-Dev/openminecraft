#include <SDL3/SDL_error.h>

#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/fontproc/om_font.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/io/om_io_utils.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/renderer/common/om_renderer_shadercompiler.hpp"
#include "openminecraft/renderer/common/shader/om_renderer_shadercompiler_shaderc.hpp"
#include "openminecraft/specs/bmp/om_bmp.hpp"
#include "openminecraft/specs/jfif/om_jfif.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/specs/zlib/om_zlib_inflate.hpp"
#include "openminecraft/util/om_util_bitbuffer.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <boost/stacktrace/stacktrace.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <string>

#include <SDL3/SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>
#include <thread>

using namespace openminecraft;
using namespace openminecraft::vm;
using namespace openminecraft::binary::hash;
using namespace openminecraft::renderer;
using namespace std::chrono_literals;

namespace openminecraft::boot
{
static void setupI18nEnv()
{
    i18n::res::registerModule("openminecraft-boot");
    i18n::res::registerModule("openminecraft-renderer");
    i18n::res::pushResourceRoot("/bootassets");
    i18n::res::load();
}

int boot(std::vector<std::string> args)
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

    std::string comm;
    while (true)
    {
        std::cout << "pixeltower shell > ";
        std::cin >> comm;

        switch (hash_compile_time(comm.c_str()))
        {
        case "vktest"_hash: {
            vulkanRendererTest();
            break;
        }
        case "gltest"_hash: {
            openglRendererTest();
            break;
        }
        case "quit"_hash:
        case "exit"_hash:
            goto progEnd;
        case "font"_hash: {
            auto iff = vfs::fsfetch("/bootassets/openminecraft-boot/font/StarRailFont.ttf");
            auto f = new fontproc::OMFont(*iff.get());
            f->buildBasicPolygon(0x2609);
            f->buildBasicPolygon('8');
            delete f;
            break;
        }
        case "dumptrace"_hash:
            logger->dumpStacktrace();
            break;
        case "dumpmem"_hash:
            mem::castorice::printres();
            break;
        case "pt_buildcls"_hash: {
            pixeltowerDynTest();
            break;
        }
        case "ptinit"_hash: {
            pixeltowerLoadTest();
            break;
        }
        case "crash"_hash: {
            logger->info("{}", *reinterpret_cast<int *>(33550336));
        }
        case "jpg"_hash: {
            auto ist = std::make_shared<std::ifstream>("/home/coder2/raw.jpg", std::ios::binary);
            specs::jfif::OMJfifFile pf;
            pf.parse(ist);
            std::ofstream oo("test2.bin", std::ios::binary);
            oo.write(reinterpret_cast<char *>(pf.getData()), pf.getWidth() * pf.getHeight() * 4);
            oo.close();
            break;
        }
        case "bmp"_hash: {
            auto ist = std::make_shared<std::ifstream>("/home/coder2/this.bmp", std::ios::binary);
            specs::bmp::OMBmpFile pf(ist);
            logger->info("test!");

            break;
        }
        case "png"_hash: {
            auto ist = std::make_shared<std::ifstream>("/home/coder2/this1.png", std::ios::binary);
            specs::png::OMPngFile pf;
            pf.parse(ist);
            logger->info("test!");

            std::ofstream oo("test2.bin", std::ios::binary);
            oo.write(reinterpret_cast<char *>(pf.fetchData()), 3000 * 4567 * 4);
            oo.close();
            break;
        }
        case "inflate"_hash: {
            specs::zlib::OMZLibInflater inf(
                [&](uint8_t *data, uint64_t length) { logger->info("{}", std::string((char *)data, length)); });
            auto ist = std::make_shared<std::ifstream>("/home/coder2/compressed", std::ios::binary);

            uint8_t buff[64];
            while (true)
            {
                auto ll = ist->readsome(reinterpret_cast<char *>(buff), 64);
                if (ll == 0)
                {
                    break;
                }
                inf.input(buff, ll);
            }
            break;
        }
        case "shd"_hash: {
            auto target = vfs::fsfetch("/bootassets/openminecraft-renderer/shaders/simple.vert.glsl");
            auto comp = common::OMRendererShaderCompiler();
            comp.install(std::make_shared<common::OMRendererShaderCompilerBackendShaderc>());
            for (int i = 0; i < 1024 * 8; i++)
            {
                comp.addCompileTask(std::make_shared<common::OMShader>(common::GLSLSource, io::readOnce(target.get()),
                                                                       "simple.vert.glsl", "main", common::Vertex));
            }
            while (true)
            {
                logger->info("{}", comp.getCompleteRatio());
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                if (comp.getCompleteRatio() >= 1.0f)
                {
                    break;
                }
            }
            break;
        }
        case "bin"_hash: {
            util::OMBitBuffer buf;
            buf.push(0b11001101);
            logger->info("{}", buf.popValue(4));
            break;
        }
        default:
            logger->warn("unknown command!");
            break;
        }
    }

progEnd:
    SDL_Quit();
    mem::castorice::printres();

    return 0;
}

} // namespace openminecraft::boot
