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
#include <cmath>
#include <iomanip>

#include "openminecraft-shell/om_application.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_textinput.hpp"
#include "openminecraft-shell/om_application_terrain.hpp"

using namespace openminecraft;
using namespace openminecraft::renderer::common;
using namespace openminecraft::renderer;

namespace openminecraft::shell
{
void OMApplication::mainLoop(OMBackend backend)
{
    auto logger = std::make_shared<log::OMLogger>("Test Renderer");

    std::atomic<bool> stopProducing{false};
    std::thread producer;

    struct ProducerJoiner
    {
        std::atomic<bool> &stop;
        std::thread &thr;

        ~ProducerJoiner()
        {
            stop.store(true);
            if (thr.joinable())
            {
                thr.join();
            }
        }
    } producerJoiner{stopProducing, producer};

    try
    {
        OMWindowConfig conf = {backend, false, 240, 854, 480};
        // INFO: requires at least OpenGL 4.0 Core Profile or Vulkan 1.2
        OMWindow win({util::OMVersion(4, 0, 0, 0), util::OMVersion(1, 2, 0, 0)}, conf,
                     "/bootassets/openminecraft-renderer/shaders");

        using OMEventBusSDL = event::OMEventBus<SDL_EventType, SDL_Event>;
        OMEventBusSDL bus;
        event::OMEventBusWrap buswrap;

        const auto startupCam = application::startupCamera();

        auto camera = std::make_shared<basics::OMCamera>(win(), startupCam.chunkX, startupCam.chunkY,
                                                         startupCam.chunkZ, startupCam.localX, startupCam.localY,
                                                         startupCam.localZ, startupCam.yaw, startupCam.pitch);

        auto chunkManager = std::make_shared<world::OMChunkManager<16>>();

        producer = std::thread([chunkManager, &stopProducing]() {
            application::runTerrainGeneration(*chunkManager, stopProducing);
        });

        bool inGame = false;
        bool mainScreen = true;
        buswrap.append(event::Custom, [&](event::OMEvent &e) {
            logger->debug("Received event {:x}", e.custom.flag);
            switch (e.custom.flag)
            {
            case EventMainScreenOpen:
                mainScreen = true;
                inGame = false;
                break;
            case EventMainScreenCloseFinish:
                mainScreen = false;
                break;
            case EventQuit:
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
        hnd2->chunkFetch = [&]() { return hnd->voxelManager->fetchChunkState(); };
        hnd2->gameTickFetch = [&]() { return hnd->getTick(); };
        hnd->voxelManager->fetchLayerState();
        win()->registerHandler(hnd2);
        win()->registerHandler(hnd4);
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

        glm::vec3 movingSpeed = {0.0f, 0.0f, 0.0f};
        bus.appendGeneral([&]() {
            static auto startTime = std::chrono::high_resolution_clock::now();
            const auto currentTime = std::chrono::high_resolution_clock::now();
            const float time = std::chrono::duration<float>(currentTime - startTime).count();
            startTime = currentTime;

            constexpr float TICK_RATE = 20.0f;
            constexpr float TICK_DT = 1.0f / TICK_RATE;

            constexpr float FRICTION_GROUND = 0.546f;
            constexpr float FRICTION_AIR = 0.91f;
            constexpr float ACCEL_WALK = 0.3f;
            constexpr float MAX_SPEED_TICK = 1.21585f;

            static float accumulator = 0.0f;
            accumulator += time;

            while (accumulator >= TICK_DT)
            {
                if (openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeTextInput::isTextInputFocused())
                {
                    keystates.fill(false);
                }

                float inputX = (keystates[0] ? 1.0f : 0.0f) - (keystates[2] ? 1.0f : 0.0f);
                float inputZ = (keystates[3] ? 1.0f : 0.0f) - (keystates[1] ? 1.0f : 0.0f);
                float len = std::sqrt(inputX * inputX + inputZ * inputZ);
                if (len > 0.0f)
                {
                    inputX /= len;
                    inputZ /= len;
                }

                bool onGround = false;
                float friction = onGround ? FRICTION_GROUND : FRICTION_AIR;
                movingSpeed.x *= friction;
                movingSpeed.z *= friction;
                movingSpeed.y *= FRICTION_AIR;

                float accel = ACCEL_WALK * (onGround ? 1.0f : 0.2f);
                movingSpeed.x += inputX * accel;
                movingSpeed.z += inputZ * accel;

                float horizSpeed = std::sqrt(movingSpeed.x * movingSpeed.x + movingSpeed.z * movingSpeed.z);
                if (horizSpeed > MAX_SPEED_TICK)
                {
                    movingSpeed.x *= MAX_SPEED_TICK / horizSpeed;
                    movingSpeed.z *= MAX_SPEED_TICK / horizSpeed;
                }

                accumulator -= TICK_DT;

                float frictionY = onGround ? FRICTION_GROUND : FRICTION_AIR;
                movingSpeed.y *= frictionY;

                float accelY = ACCEL_WALK * (onGround ? 1.0f : 0.2f);

                if (keystates[5])
                {
                    movingSpeed.y = fmin(movingSpeed.y + accelY, MAX_SPEED_TICK);
                }
                else if (keystates[4])
                {
                    movingSpeed.y = fmax(movingSpeed.y - accelY, -MAX_SPEED_TICK);
                }

                movingSpeed.y = std::clamp(movingSpeed.y, -MAX_SPEED_TICK, MAX_SPEED_TICK);
            }

            camera->moveCamera(basics::OMCameraMovement::Forward, movingSpeed.x * TICK_RATE * time);
            camera->moveCamera(basics::OMCameraMovement::Right, movingSpeed.z * TICK_RATE * time);
            camera->moveCamera(basics::OMCameraMovement::Up, movingSpeed.y * TICK_RATE * time);
        });

        bus.append(SDL_EVENT_MOUSE_MOTION, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::MouseMotion;
            const float uiScale = std::max(0.01f, win()->getUiScale());
            ev.mousemotion.x = e.motion.x / uiScale;
            ev.mousemotion.y = e.motion.y / uiScale;
            ev.mousemotion.button = -1;
            buswrap.handle(event::MouseMotion, ev);
        });
        bus.append(SDL_EVENT_MOUSE_BUTTON_UP, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::MouseUp;
            const float uiScale = std::max(0.01f, win()->getUiScale());
            ev.mousebutton.x = e.button.x / uiScale;
            ev.mousebutton.y = e.button.y / uiScale;
            ev.mousebutton.button = e.button.button;
            buswrap.handle(event::MouseUp, ev);
        });
        bus.append(SDL_EVENT_MOUSE_BUTTON_DOWN, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::MouseDown;
            const float uiScale = std::max(0.01f, win()->getUiScale());
            ev.mousebutton.x = e.button.x / uiScale;
            ev.mousebutton.y = e.button.y / uiScale;
            ev.mousebutton.button = e.button.button;
            buswrap.handle(event::MouseDown, ev);
        });
        bus.append(SDL_EVENT_MOUSE_WHEEL, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::MouseWheel;
            const float uiScale = std::max(0.01f, win()->getUiScale());
            // SDL wheel x/y are scroll deltas; the cursor position is mouse_x/mouse_y.
            ev.mousewheel.x = e.wheel.mouse_x / uiScale;
            ev.mousewheel.y = e.wheel.mouse_y / uiScale;
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

        bus.append(SDL_EVENT_TEXT_INPUT, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::TextInput;
            ev.text = e.text.text != nullptr ? e.text.text : "";
            buswrap.handle(event::TextInput, ev);
        });

        bus.append(SDL_EVENT_TEXT_EDITING, [&](SDL_Event &e) -> void {
            event::OMEvent ev;
            ev.type = event::TextEditing;
            ev.text = e.edit.text != nullptr ? e.edit.text : "";
            ev.selectionStart = e.edit.start;
            ev.selectionLength = e.edit.length;
            buswrap.handle(event::TextEditing, ev);
        });
        bus.append(SDL_EVENT_FINGER_MOTION, [&](SDL_Event &e) {
            int w, h;
            SDL_GetWindowSize(SDL_GetWindowFromEvent(&e), &w, &h);
            event::OMEvent ev;
            ev.type = event::MouseMotion;
            const float uiScale = std::max(0.01f, win()->getUiScale());
            ev.mousemotion.x = e.tfinger.x * w / uiScale;
            ev.mousemotion.y = e.tfinger.y * h / uiScale;
            ev.mousemotion.button = e.tfinger.fingerID;
            buswrap.handle(event::MouseMotion, ev);
        });
        bus.append(SDL_EVENT_FINGER_UP, [&](SDL_Event &e) {
            int w, h;
            SDL_GetWindowSize(SDL_GetWindowFromEvent(&e), &w, &h);
            event::OMEvent ev;
            ev.type = event::MouseUp;
            const float uiScale = std::max(0.01f, win()->getUiScale());
            ev.mousebutton.x = e.tfinger.x * w / uiScale;
            ev.mousebutton.y = e.tfinger.y * h / uiScale;
            ev.mousebutton.button = e.tfinger.fingerID;
            buswrap.handle(event::MouseUp, ev);
        });
        bus.append(SDL_EVENT_FINGER_DOWN, [&](SDL_Event &e) {
            int w, h;
            SDL_GetWindowSize(SDL_GetWindowFromEvent(&e), &w, &h);
            event::OMEvent ev;
            ev.type = event::MouseDown;
            const float uiScale = std::max(0.01f, win()->getUiScale());
            ev.mousebutton.x = e.tfinger.x * w / uiScale;
            ev.mousebutton.y = e.tfinger.y * h / uiScale;
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

            if (openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeTextInput::isTextInputFocused())
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
            if (openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeTextInput::isTextInputFocused())
            {
                return;
            }

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
            win.applyTextInputState();

            win()->render(ticker);
        }

        logger->info("exiting!");

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
} // namespace openminecraft::shell
