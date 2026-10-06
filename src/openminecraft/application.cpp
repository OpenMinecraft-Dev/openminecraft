#include <SDL3/SDL_error.h>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_mouse.h"
#include "openminecraft-shell/data/block/om_block_registery.hpp"
#include "openminecraft-shell/data/block/om_blockstate_registry.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft-shell/renderer/composerenderer.hpp"
#include "openminecraft-shell/renderer/surfacerenderer.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/network/om_network_dnsquery.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/om_renderer_window.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <array>
#include <boost/stacktrace/stacktrace.hpp>
#include "openminecraft/renderer/common/event/om_eventbus.hpp"
#include "openminecraft/renderer/common/event/om_eventbus_wrap.hpp"
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include "openminecraft-shell/renderer/debugrendrer.hpp"
#include "openminecraft-shell/renderer/worldrenderer.hpp"
#include "openminecraft/world/om_world_chunkmanager.hpp"
#include "openminecraft-shell/network/om_protocol_minecraft.hpp"

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

    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "OpenMinecraft");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, OM_VERSION);
    SDL_SetHint(SDL_HINT_APP_NAME, "OpenMinecraft");

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
            logger->info("gl -> OpenGL Backend (3.3 Core Profile)");
            goto end;
        }
        bk = args[1] == "gl" ? OpenGL : Vulkan;
    }

    data::block::registerBlocks();
    data::block::registerBlockstates();
    // networkSetup();
    mainLoop(bk);

end:
    SDL_Quit();

    return 0;
}

void OMApplication::networkSetup()
{
    std::string host = "MinecraftOnline.com";
    std::string port = "25565";
    logger.info("try connect to {} {}", host, port);

    auto res = openminecraft::network::queryDns(std::string("_minecraft._tcp.") + host);
    for (const auto &r : res)
    {
        host = r.target;
        port = std::to_string(r.port);
        logger.info("=> {}:{}", host, port);
    }

    vfs::fsmountTcp(host, port, "/mcserver_conn");
    auto in = vfs::fsfetch("/mcserver_conn/connect");
    auto out = vfs::fswrite("/mcserver_conn/connect");

    openminecraftshell::network::OMProtocolMinecraftHandler hnd;
    openminecraftshell::network::OMProtocolMinecraft protocol(in, out, hnd);

    try
    {
        protocol.write(openminecraft::network::OMNetworkPacket()
                           .uint8(0x00)
                           .varInt(773)
                           .utf8WithLength("localhost")
                           .int16(25565)
                           .varInt(1));

        protocol.write(openminecraft::network::OMNetworkPacket().uint8(0x00));
        protocol.write(openminecraft::network::OMNetworkPacket().uint8(0x01).int64(time(nullptr)));

        auto p = protocol.read();
        logger.debug("packet length {}", p.datalen());

        vfs::fsumount("/mcserver_conn");
    }
    catch (const std::ios_base::failure &e)
    {
        logger.error("connection closed!");
    }
}

void OMApplication::mainLoop(OMBackend backend)
{
    auto logger = std::make_shared<log::OMLogger>("Test Renderer");
    try
    {
        OMWindowConfig conf = {backend, false, 240, 854, 480};
        // INFO: requires at least OpenGL 4.0 Core Profile or Vulkan 1.2
        OMWindow win({util::Version(4, 0, 0, 0), util::Version(1, 2, 0, 0)}, conf,
                     "/bootassets/openminecraft-renderer/shaders");

        using OMEventBusSDL = event::OMEventBus<SDL_EventType, SDL_Event>;
        OMEventBusSDL bus;
        event::OMEventBusWrap buswrap;
        auto camera = std::make_shared<basics::OMCamera>(win(), glm::vec3{-1.0f, 5.0f, -1.0f}, 45.0f, -45.0f);

        auto chunkManager = std::make_shared<world::OMChunkManager<16>>();
        int width = 16;
        int height = 16;
        int depth = 8;
        for (int cx = -width; cx < width; ++cx)
        {
            for (int cy = -depth; cy <= 0; ++cy)
            {
                for (int cz = -height; cz < height; ++cz)
                {
                    using data::block::blockstateRegistry;
                    world::OMChunk<16> cnk(cx, cy, cz);
                    cnk.setBlock(0, 1, 0,
                                 blockstateRegistry.id(data::OMIdentifier("minecraft:water[falling=false,level=1]")));
                    cnk.setBlock(0, 0, 0,
                                 blockstateRegistry.id(data::OMIdentifier("minecraft:lava[falling=false,level=0]")));
                    cnk.setBlock(0, 15, 0,
                                 blockstateRegistry.id(data::OMIdentifier("minecraft:water[falling=true,level=8]")));
                    cnk.setBlock(15, 1, 0,
                                 blockstateRegistry.id(data::OMIdentifier("minecraft:water[falling=false,level=2]")));
                    cnk.setBlock(0, 1, 15,
                                 blockstateRegistry.id(data::OMIdentifier("minecraft:water[falling=false,level=2]")));
                    cnk.setBlock(15, 1, 15,
                                 blockstateRegistry.id(data::OMIdentifier("minecraft:water[falling=false,level=3]")));
                    cnk.setBlock(15, 0, 0,
                                 blockstateRegistry.id(data::OMIdentifier("minecraft:white_stained_glass[]")));
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
                            "minecraft:cherry_stairs[facing=east,half=bottom,shape=straight,water_logged=true]")));
                    cnk.setBlock(3, 1, 0,
                                 blockstateRegistry.id(data::OMIdentifier(
                                     "minecraft:cherry_door[facing=east,half=lower,hinge=left,open=false]")));
                    cnk.setBlock(3, 2, 0,
                                 blockstateRegistry.id(data::OMIdentifier(
                                     "minecraft:cherry_door[facing=east,half=upper,hinge=left,open=false]")));
                    cnk.setBlock(0, 2, 1,
                                 blockstateRegistry.id(data::OMIdentifier(
                                     "minecraft:cherry_hanging_sign[attached=false,rotation=3,water_logged=false]")));
                    cnk.setBlock(0, 1, 2, blockstateRegistry.id(data::OMIdentifier("minecraft:copper_ore[]")));
                    cnk.setBlock(15, 1, 1, blockstateRegistry.id(data::OMIdentifier("minecraft:copper_ore[]")));
                    cnk.setBlock(15, 1, 2, blockstateRegistry.id(data::OMIdentifier("minecraft:copper_ore[]")));
                    cnk.setBlock(15, 2, 2,
                                 blockstateRegistry.id(
                                     data::OMIdentifier("minecraft:cherry_fence_gate[facing=south,in_wall=false,"
                                                        "open=true,powered=true,water_logged=false]")));
                    cnk.setBlock(
                        15, 2, 1,
                        blockstateRegistry.id(data::OMIdentifier("minecraft:stone_pressure_plate[powered=false]")));
                    chunkManager->loadChunk(cnk);
                }
            }
        }

        bool mainScreen = true;
        buswrap.append(event::Custom, [&](event::OMEvent &e) {
            logger->debug("Received event {:x}", e.custom.flag);
            switch (e.custom.flag)
            {
            case 3:
                mainScreen = true;
                break;
            case 4:
                mainScreen = false;
                break;
            case 2:
                isRunning = false;
                break;
            default:
                break;
            }
        });

        auto hnd4 = std::make_shared<renderer::OMSurfaceRenderer>(win(), buswrap);
        auto hnd2 = std::make_shared<renderer::OMDebugRenderer>(win(), buswrap);
        auto hnd = std::make_shared<renderer::OMWorldRenderer>(win(), camera, chunkManager);
        auto hnd3 = std::make_shared<renderer::OMComposeRenderer>(win(), hnd4->internal->middleTarget,
                                                                  hnd2->internal->middleTarget, hnd->tempTarget);
        hnd2->camera = camera.get();
        hnd2->stateFetch = [&]() { return hnd->voxelManager->fetchLayerState(); };
        hnd->voxelManager->fetchLayerState();
        win()->registerHandler(hnd2);
        win()->registerHandler(hnd4);
        win()->registerHandler(hnd);
        win()->registerHandler(hnd3);
        win()->baseInit();

        bool inGame = false;
        std::array<bool, 6> keystates = {false, false, false, false, false, false};
        bus.append(SDL_EVENT_WINDOW_RESIZED, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_WINDOW_METAL_VIEW_RESIZED, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_WINDOW_ENTER_FULLSCREEN, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_WINDOW_LEAVE_FULLSCREEN, [&](SDL_Event &) -> void { win()->requestResize(); });
        bus.append(SDL_EVENT_QUIT, [&](SDL_Event &) -> void { isRunning = false; });
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

            constexpr float moveSpeed = 40.3f;

            if (!inGame)
            {
                return;
            }

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

        bus.append(SDL_EVENT_MOUSE_MOTION, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::MouseMotion;
            ev.mousemotion.x = e.motion.x;
            ev.mousemotion.y = e.motion.y;
            ev.mousemotion.button = -1;
            buswrap.handle(event::MouseMotion, ev);
        });
        bus.append(SDL_EVENT_MOUSE_BUTTON_UP, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::MouseUp;
            ev.mousebutton.x = e.button.x;
            ev.mousebutton.y = e.button.y;
            ev.mousebutton.button = e.button.button;
            buswrap.handle(event::MouseUp, ev);
        });
        bus.append(SDL_EVENT_MOUSE_BUTTON_DOWN, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::MouseDown;
            ev.mousebutton.x = e.button.x;
            ev.mousebutton.y = e.button.y;
            ev.mousebutton.button = e.button.button;
            buswrap.handle(event::MouseDown, ev);
        });
        bus.append(SDL_EVENT_MOUSE_WHEEL, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::MouseWheel;
            ev.mousewheel.x = e.wheel.x;
            ev.mousewheel.y = e.wheel.y;
            ev.mousewheel.wheelx = e.wheel.x;
            ev.mousewheel.wheely = e.wheel.y;
            buswrap.handle(event::MouseWheel, ev);
        });
        bus.append(SDL_EVENT_KEY_DOWN, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::KeyDown;
            ev.key.keycode = e.key.key;
            ev.key.modifier = e.key.mod;
            buswrap.handle(event::KeyDown, ev);
        });
        bus.append(SDL_EVENT_KEY_UP, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::KeyUp;
            ev.key.keycode = e.key.key;
            ev.key.modifier = e.key.mod;
            buswrap.handle(event::KeyUp, ev);
        });
        bus.append(SDL_EVENT_FINGER_MOTION, [&](SDL_Event &e) {
            int w, h;
            SDL_GetWindowSize(SDL_GetWindowFromEvent(&e), &w, &h);
            event::OMEvent ev;
            ev.type = event::MouseMotion;
            ev.mousemotion.x = e.tfinger.x * w;
            ev.mousemotion.y = e.tfinger.y * h;
            ev.mousemotion.button = e.tfinger.fingerID;
            buswrap.handle(event::MouseMotion, ev);
        });
        bus.append(SDL_EVENT_FINGER_UP, [&](SDL_Event &e) {
            int w, h;
            SDL_GetWindowSize(SDL_GetWindowFromEvent(&e), &w, &h);
            event::OMEvent ev;
            ev.type = event::MouseUp;
            ev.mousebutton.x = e.tfinger.x * w;
            ev.mousebutton.y = e.tfinger.y * h;
            ev.mousebutton.button = e.tfinger.fingerID;
            buswrap.handle(event::MouseUp, ev);
        });
        bus.append(SDL_EVENT_FINGER_DOWN, [&](SDL_Event &e) {
            int w, h;
            SDL_GetWindowSize(SDL_GetWindowFromEvent(&e), &w, &h);
            event::OMEvent ev;
            ev.type = event::MouseDown;
            ev.mousebutton.x = e.tfinger.x * w;
            ev.mousebutton.y = e.tfinger.y * h;
            ev.mousebutton.button = e.tfinger.fingerID;
            buswrap.handle(event::MouseDown, ev);
        });

        hnd2->node->bindEventBus(buswrap);
        hnd4->node->bindEventBus(buswrap);
        bus.append(SDL_EVENT_KEY_DOWN, [&](SDL_Event &e) -> void {
            if (e.key.repeat)
            {
                return;
            }

            switch (e.key.key)
            {
            case SDLK_ESCAPE:
                inGame = false;
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
            if (e.button.button == 1 && !mainScreen)
            {
                inGame = true;
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
            SDL_SetWindowRelativeMouseMode(reinterpret_cast<SDL_Window *>(*win), inGame);

            win()->render(ticker);
        }

        hnd = nullptr;
        hnd2 = nullptr;
        hnd3 = nullptr;
        hnd4 = nullptr;
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
