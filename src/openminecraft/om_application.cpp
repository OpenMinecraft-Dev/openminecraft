#include "openminecraft/compiler/om_compiler_utils.hpp"
#include <optional>
#define NOMINMAX
#include <SDL3/SDL_error.h>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_mouse.h"
#include "glm/ext/vector_float3.hpp"
#include "openminecraft-shell/data/biome/om_biome_registry.hpp"
#include "openminecraft-shell/data/block/om_block_registry.hpp"
#include "openminecraft-shell/data/block/om_blockstate_registry.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft-shell/renderer/om_compose_renderer.hpp"
#include "openminecraft-shell/renderer/om_surface_renderer.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/om_renderer_window.hpp"
#include "openminecraft/specs/nbt/om_nbt_reader.hpp"
#include "openminecraft/specs/zlib/om_zlib_deflate.hpp"
#include "openminecraft/specs/zlib/om_zlib_inflaterstream.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <algorithm>
#include <array>
#include <boost/stacktrace/stacktrace.hpp>
#include "openminecraft/renderer/common/event/om_eventbus.hpp"
#include "openminecraft/renderer/common/event/om_eventbus_wrap.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <cmath>
#include <stdexcept>
#include <string>
#include <cstdlib>
#include <thread>
#include <filesystem>
#include "openminecraft-shell/renderer/om_debug_renderer.hpp"
#include "openminecraft-shell/renderer/om_world_renderer.hpp"
#include "openminecraft/world/om_world_chunkmanager.hpp"

#include <SDL3/SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>
#include <vector>
#include <iomanip>

#include "openminecraft-shell/om_application.hpp"

#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_token.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>

#include "openminecraft/compiler/gsl/om_compiler_tokeniter_gsl.hpp"

using namespace openminecraft;
using namespace openminecraft::renderer::common;
using namespace openminecraft::renderer;

namespace openminecraft::shell
{
struct MyPreprocessingHooks : boost::wave::context_policies::default_preprocessing_hooks
{
    MyPreprocessingHooks() = default;

    template <typename ContextT>
    auto locate_include_file(ContextT &ctx, std::string &file_path, bool is_system, char const *current_name,
                             std::string &dir_path, std::string &native_name) -> bool
    {
        if (vfs::fsfetch("/bootassets/openminecraft-renderer/shaders/" + file_path))
        {
            native_name = file_path;
            dir_path = "";
            return true;
        }

        return false;
    }
};

std::vector<std::string> storage;
class memory_input_policy
{
  public:
    template <typename IterContext> class inner
    {
      public:
        template <typename Position>
        void init_iterators(IterContext &iter_ctx, Position const &act_pos, boost::wave::language_support language)
        {
            using iterator_type = typename IterContext::iterator_type;

            const std::string &id = std::string(iter_ctx.filename.c_str());
            auto pth = "/bootassets/openminecraft-renderer/shaders/" + id;
            auto istr = vfs::fsfetch(pth);

            auto cnt = compiler::readOnce(istr.get());
            std::string data(cnt.begin(), cnt.end());

            storage.emplace_back(data);

            auto &content = storage.back();
            iter_ctx.first =
                typename IterContext::iterator_type(content.begin(), content.end(), Position(pth.c_str()), language);
            iter_ctx.last = typename IterContext::iterator_type();
        }
    };
};

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

    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "OpenMinecraft");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, OM_VERSION);
    SDL_SetHint(SDL_HINT_APP_NAME, "OpenMinecraft");
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    // macOS: the default "press and hold" accent menu prevents key repeat, which
    // breaks held-key text input. Disable it so holding a key emits repeats.
    SDL_SetHint(SDL_HINT_MAC_PRESS_AND_HOLD, "0");

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

    OMBackend bk = Vulkan;
    if (args.size() >= 2)
    {
        if (args[1] == "help")
        {
            logger->info("OpenMinecraft Demo App {}-{}", OM_VERSION, OM_VERSION_CHANNEL);
            logger->info("usage: [backend]");
            logger->info("possible backends:");
            logger->info("vk -> Vulkan Backend (1.2, Default)");
            logger->info("gl -> OpenGL Backend (4.0 Core Profile)");
            goto end;
        }
        else if (args[1] == "network")
        {
            networkSetup(args[2], args[3]);
            goto end;
        }
        else if (args[1] == "nbt")
        {
            specs::nbt::OMNbtReader reader(std::make_shared<std::ifstream>(args[2], std::ios::binary));
            reader.parse(false);
            goto end;
        }
        else if (args[1] == "json")
        {
            compiler::json::OMJsonAstBuilder bld(
                std::make_shared<compiler::json::OMJsonTokenIter>(std::make_shared<std::ifstream>(args[2])));
            auto document = bld.build();
            logger->debug("doc at {}", (void *)document.get());
            goto end;
        }
        else if (args[1] == "zlib")
        {
            specs::zlib::OMZLibDeflater def([](uint8_t *data, uint64_t l) {
                for (int i = 0; i < l; ++i)
                {
                    std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)data[i] << " ";
                }
                std::cout << std::endl;
            });
            def.input(reinterpret_cast<uint8_t *>(const_cast<char *>(args[2].c_str())), args[2].size());
            def.finish();
            goto end;
        }
        else if (args[1] == "blk")
        {
            compiler::json::OMJsonAstBuilder bld(
                std::make_shared<compiler::json::OMJsonTokenIter>(std::make_shared<specs::zlib::OMZlibInflaterStream>(
                    vfs::fsfetch("/bootassets/minecraft/blocks.json.zlib"))));
            auto document = bld.build();

            for (const auto &p : document->getMap())
            {
                logger->debug(p.first);
            }

            goto end;
        }
        else if (args[1] == "prep")
        {
            try
            {
                auto iff = std::make_shared<std::ifstream>(args[2]);

                auto itt = std::make_shared<compiler::gsl::OMGslTokenIter>(
                    iff,
                    [](std::string name) -> std::optional<std::string> {
                        auto f = vfs::fsfetch("/bootassets/openminecraft-renderer/shaders/" + name);
                        if (f == nullptr)
                        {
                            return std::nullopt;
                        }
                        std::string input((std::istreambuf_iterator<char>(*f)), std::istreambuf_iterator<char>());
                        return input;
                    },
                    args[2]);

                while (!itt->end())
                {
                    std::cout << itt->next()->content;
                }
            }
            catch (boost::wave::cpp_exception const &e)
            {
                std::cerr << "Wave Error: " << e.description() << std::endl;
                std::cerr << "  File: " << e.file_name() << std::endl;
                std::cerr << "  Line: " << e.line_no() << ", Column: " << e.column_no() << std::endl;
                std::cerr << "  Error code: " << e.get_errorcode() << std::endl;
                std::cerr << "  Severity: " << e.get_severity() << std::endl;
            }
            goto end;
        }
        bk = args[1] == "gl" ? OpenGL : Vulkan;
    }

    data::block::registerBlocks();
    data::block::registerBlockStates();
    data::biome::registerBiomes();
    mainLoop(bk);

end:
    SDL_Quit();

    return 0;
}

} // namespace openminecraft::shell
