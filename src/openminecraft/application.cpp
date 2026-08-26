#include <SDL3/SDL_error.h>

#include <algorithm>
#include "SDL3/SDL_events.h"
#include "openminecraft-shell/data/block/om_block.hpp"
#include "openminecraft-shell/data/block/om_block_registery.hpp"
#include "openminecraft-shell/data/block/om_blockstate_registry.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft-shell/renderer/composerenderer.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/om_renderer_window.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <boost/stacktrace/stacktrace.hpp>
#include "openminecraft/renderer/common/event/om_eventbus.hpp"
#include <memory>
#include <string>
#include "openminecraft-shell/renderer/debugrendrer.hpp"
#include "openminecraft-shell/renderer/worldrenderer.hpp"

#include <SDL3/SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>
#include <vector>

#include "openminecraft-shell/application.hpp"

using namespace openminecraft;
using namespace openminecraft::renderer::common;
using namespace openminecraft::renderer;

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

    OMBackend bk = Vulkan;
    if (args.size() >= 2)
    {
        bk = args[1] == "gl" ? OpenGL : Vulkan;
    }

    data::block::registerBlocks();
    data::block::registerBlockstates();
    mainLoop(bk);

    SDL_Quit();

    return 0;
}

void OMApplication::mainLoop(OMBackend backend)
{
    auto logger = std::make_shared<log::OMLogger>("Test Renderer");
    try
    {
        OMWindowConfig conf = {backend, false, 240, 1024, 768};
        // INFO: requires at least OpenGL 3.3 Core Profile or Vulkan 1.2
        OMWindow win({util::Version(3, 3, 0, 0), util::Version(1, 2, 0, 0)}, conf,
                     "/bootassets/openminecraft-renderer/shaders");

        event::OMEventBusSDL bus;
        auto camera = std::make_shared<basics::OMCamera>(win(), glm::vec3{-1.0f, 5.0f, -1.0f}, 45.0f, -45.0f);

        auto chunkManager = std::make_shared<world::OMChunkManager<16>>();
        for (int cx = -1; cx < 8; ++cx)
        {
            for (int cy = 0; cy < 8; ++cy)
            {
                for (int cz = -1; cz < 8; ++cz)
                {
                    using data::block::blockstateRegistry;
                    world::OMChunk<16> cnk(cx, cy, cz);
                    cnk.setBlock(0, 0, 0, blockstateRegistry.id(data::OMIdentifier("minecraft:stone[]")));
                    cnk.setBlock(0, 1, 1, blockstateRegistry.id(data::OMIdentifier("minecraft:copper_ore[]")));
                    cnk.setBlock(2, 0, 0,
                                 blockstateRegistry.id(data::OMIdentifier("minecraft:grass_block[snowy=false]")));
                    cnk.setBlock(2, 1, 0,
                                 blockstateRegistry.id(data::OMIdentifier("minecraft:tall_grass[half=lower]")));
                    cnk.setBlock(2, 2, 0,
                                 blockstateRegistry.id(data::OMIdentifier("minecraft:tall_grass[half=upper]")));
                    cnk.setBlock(
                        1, 1, 0,
                        blockstateRegistry.id(data::OMIdentifier(
                            "minecraft:cherry_stairs[facing=east,half=bottom,shape=straight,water_logged=false]")));
                    cnk.setBlock(3, 1, 0,
                                 blockstateRegistry.id(data::OMIdentifier(
                                     "minecraft:cherry_door[facing=east,half=lower,hinge=left,open=false]")));
                    cnk.setBlock(3, 2, 0,
                                 blockstateRegistry.id(data::OMIdentifier(
                                     "minecraft:cherry_door[facing=east,half=upper,hinge=left,open=false]")));
                    cnk.setBlock(0, 1, 0,
                                 blockstateRegistry.id(data::OMIdentifier(
                                     "minecraft:cherry_shelf[facing=east,powered=false,side_chain=unconnected]")));
                    cnk.setBlock(0, 2, 1,
                                 blockstateRegistry.id(data::OMIdentifier(
                                     "minecraft:cherry_hanging_sign[attached=false,rotation=3,water_logged=false]")));
                    cnk.setBlock(0, 1, 2, blockstateRegistry.id(data::OMIdentifier("minecraft:copper_ore[]")));
                    cnk.setBlock(15, 1, 1, blockstateRegistry.id(data::OMIdentifier("minecraft:copper_ore[]")));
                    cnk.setBlock(15, 0, 0, blockstateRegistry.id(data::OMIdentifier("minecraft:cobblestone[]")));
                    cnk.setBlock(15, 1, 2, blockstateRegistry.id(data::OMIdentifier("minecraft:copper_ore[]")));
                    cnk.setBlock(15, 1, 0,
                                 blockstateRegistry.id(data::OMIdentifier(
                                     "minecraft:cherry_button[face=floor,facing=south,powered=true]")));
                    cnk.setBlock(15, 2, 2,
                                 blockstateRegistry.id(
                                     data::OMIdentifier("minecraft:cherry_fence_gate[facing=south,in_wall=false,"
                                                        "open=true,powered=true,water_logged=true]")));
                    cnk.setBlock(15, 2, 1,
                                 blockstateRegistry.id(data::OMIdentifier("minecraft:rail[shape=north_south]")));
                    chunkManager->loadChunk(cnk);
                }
            }
        }

        auto hnd2 = std::make_shared<renderer::OMDebugRenderer>(win());
        auto hnd = std::make_shared<renderer::OMWorldRenderer>(win(), camera, chunkManager);
        auto hnd3 = std::make_shared<renderer::OMComposeRenderer>(
            win(), [&]() -> OMRendererTexture * { return hnd2->internal->middleTarget->colorTexture; },
            [&]() -> OMRendererTexture * { return hnd->tempTarget->colorTexture; });
        hnd2->camera = camera.get();
        win()->registerHandler(hnd2);
        win()->registerHandler(hnd);
        win()->registerHandler(hnd3);
        win()->baseInit();

        std::array<bool, 6> keystates = {false, false, false, false, false, false};
        bus.append(SDL_EVENT_WINDOW_RESIZED, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_WINDOW_METAL_VIEW_RESIZED, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_WINDOW_ENTER_FULLSCREEN, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_WINDOW_LEAVE_FULLSCREEN, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_QUIT, [&](SDL_Event &) -> void { isRunning = false; });
        bus.append(SDL_EVENT_KEY_DOWN, [&](SDL_Event &e) -> void {
            if (e.key.repeat)
            {
                return;
            }

            switch (e.key.key)
            {
            case SDLK_ESCAPE:
                SDL_SetWindowRelativeMouseMode(reinterpret_cast<SDL_Window *>(*win), false);
                break;
            case SDLK_W:
                keystates[0] = true;
                break;
            case SDLK_A:
                keystates[1] = true;
                break;
            case SDLK_S:
                keystates[2] = true;
                break;
            case SDLK_D:
                keystates[3] = true;
                break;
            case SDLK_LSHIFT:
                keystates[4] = true;
                break;
            case SDLK_SPACE:
                keystates[5] = true;
                break;
            }
        });
        bus.append(SDL_EVENT_KEY_UP, [&](SDL_Event &e) -> void {
            switch (e.key.key)
            {
            case SDLK_W:
                keystates[0] = false;
                break;
            case SDLK_A:
                keystates[1] = false;
                break;
            case SDLK_S:
                keystates[2] = false;
                break;
            case SDLK_D:
                keystates[3] = false;
                break;
            case SDLK_LSHIFT:
                keystates[4] = false;
                break;
            case SDLK_SPACE:
                keystates[5] = false;
                break;
            }
        });
        bus.append(SDL_EVENT_MOUSE_BUTTON_DOWN, [&](SDL_Event &e) -> void {
            SDL_SetWindowRelativeMouseMode(reinterpret_cast<SDL_Window *>(*win), true);
        });
        bus.append(SDL_EVENT_MOUSE_MOTION, [&](SDL_Event &e) -> void {
            if (SDL_GetWindowRelativeMouseMode(reinterpret_cast<SDL_Window *>(*win)))
            {
                camera->modPitch(-e.motion.yrel * 0.15);
                camera->modYaw(e.motion.xrel * 0.15);
            }
        });
        bus.append(SDL_EVENT_FINGER_MOTION, [&](SDL_Event &e) -> void {
            camera->modPitch(-e.tfinger.dy * 0.5f * 100.0f);
            camera->modYaw(e.tfinger.dx * 0.5f * 100.0f);
        });
        bus.appendGeneral([&]() {
            static auto startTime = std::chrono::high_resolution_clock::now();
            const auto currentTime = std::chrono::high_resolution_clock::now();
            const float time = std::chrono::duration<float>(currentTime - startTime).count();
            startTime = currentTime;

            constexpr float moveSpeed = 4.3f;

            if (keystates[0])
            {
                camera->moveCamera(basics::Forward, moveSpeed * time);
            }
            if (keystates[1])
            {
                camera->moveCamera(basics::Left, moveSpeed * time);
            }
            if (keystates[2])
            {
                camera->moveCamera(basics::Back, moveSpeed * time);
            }
            if (keystates[3])
            {
                camera->moveCamera(basics::Right, moveSpeed * time);
            }
            if (keystates[4])
            {
                camera->moveCamera(basics::Down, moveSpeed * time);
            }
            if (keystates[5])
            {
                camera->moveCamera(basics::Up, moveSpeed * time);
            }
        });

        util::OMTicker ticker;
        while (isRunning)
        {
            ticker.begin();

            SDL_Event e;
            while (SDL_PollEvent(&e))
            {
                bus.handle(static_cast<SDL_EventType>(e.type), e);
            }

            win()->render(ticker);

            hnd2->updateState(ticker);
        }

        hnd = nullptr;
        hnd2 = nullptr;
        hnd3 = nullptr;
    }
    catch (OMRendererException &e)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenMinecraft Fail", e.what(), nullptr);
    }
    catch (std::runtime_error &e)
    {
        logger->fatal(e.what());
    }
}
} // namespace openminecraftshell