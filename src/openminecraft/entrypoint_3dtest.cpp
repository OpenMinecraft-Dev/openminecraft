#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_messagebox.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/vector_float3.hpp"
#include "openminecraft-shell/renderer/entrypoint_testrenderer.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_image.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_sector.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/event/om_eventbus.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/om_renderer_window.hpp"
#include "openminecraft/util/om_util_ticker.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <map>
#include <string>
#include <vector>

using namespace openminecraft::renderer;
using namespace openminecraft::renderer::common::demiurge;

namespace openminecraftshell
{
constexpr std::array<int, 16> DebugColors = {
    (int)0xF9FFFFFF, (int)0xF9801DFF, (int)0xC74EBDFF, 0x3AB3DAFF, (int)0xFED83DFF, (int)0x80C71FFF,
    (int)0xF38BAAFF, 0x474F52FF,      (int)0x9D9D97FF, 0x169C9CFF, (int)0x8932B8FF, 0x3C44AAFF,
    (int)0x835432FF, 0x5E7C16FF,      (int)0xB02E26FF, 0x1D1D21FF,
};
class OMNodeRendererHandler : public OMRendererHandler
{
  public:
    OMNodeRendererHandler(OMRenderer *renderer) : OMRendererHandler(renderer)
    {
        this->renderer = renderer;

        fontset = std::make_shared<fontproc::OMFontSet>();

        auto rawfile2 = vfs::fsfetch("/bootassets/openminecraft-boot/font/StarRailFont.ttf");
        fontset->fontList.push_back(std::make_shared<fontproc::OMFont>(*rawfile2.get()));

        node = std::make_shared<node::OMDemiurgeRectNode>()
                   ->style({
                       {"color", (int)0x23232399},
                       {"flexGap", 10_px},
                       {"flexWrap", Wrap},
                       {"flexDirection", Column},
                       {"fill", node::OMDemiurgeImageFillType::Cover},
                       {"radius", glm::vec4(0.0f, 0.0f, 0.0f, 25.0f)},
                       {"width", 40_percent},
                       {"height", 70_percent},
                   })
                   ->mount(std::make_shared<node::OMDemiurgeContainerNode>()
                               ->style({
                                   {"flexShrink", 0.0f},
                                   {"flexGrow", 0.0f},
                                   {"flexDirection", Column},
                                   {"width", 100_percent},
                               })
                               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                           ->style({
                                               {"color", (int)0xffffffff},
                                               {"flexGrow", 1.0f},
                                               {"text", "OpenMinecraft Demo"},
                                               {"textheight", 18},
                                           }))
                               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                           ->style({
                                               {"color", (int)0xffffffff},
                                               {"flexGrow", 1.0f},
                                               {"text", ""},
                                               {"textheight", 18},
                                           })
                                           ->store(fpsTextNode))
                               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                           ->style({
                                               {"color", (int)0xffffffff},
                                               {"flexGrow", 1.0f},
                                               {"text", ""},
                                               {"textheight", 18},
                                           })
                                           ->store(posTextNode))
                               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                           ->style({
                                               {"color", (int)0xffffffff},
                                               {"flexGrow", 1.0f},
                                               {"text", ""},
                                               {"textheight", 18},
                                           })
                                           ->store(povTextNode))
                               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                           ->style({
                                               {"color", (int)0xffffffff},
                                               {"flexGrow", 1.0f},
                                               {"text", renderer->driver()},
                                               {"textheight", 18},
                                           }))
                               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                           ->style({
                                               {"color", (int)0xffffffff},
                                               {"flexGrow", 1.0f},
                                               {"text", ""},
                                               {"textheight", 18},
                                           })
                                           ->store(precisionNode))
                               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                           ->style({
                                               {"color", (int)0xffffffff},
                                               {"flexGrow", 1.0f},
                                               {"text", ""},
                                               {"textheight", 18},
                                           })
                                           ->store(precisionNode2)))
                   ->mount(std::make_shared<node::OMDemiurgeContainerNode>()
                               ->style({
                                   {"flexShrink", 0.0f},
                                   {"flexGrow", 1.0f},
                                   {"width", 100_percent},
                               })
                               ->mount(std::make_shared<node::OMDemiurgeContainerNode>()
                                           ->style({
                                               {"flexShrink", 0.0f},
                                               {"flexGrow", 1.0f},
                                           })
                                           ->store(graphNode))
                               ->store(textNode));

        internal = std::make_shared<OMDemiurgeRendererHandler>(renderer, node);
        renderer->registerHandler(internal);
    }
    ~OMNodeRendererHandler() override
    {
        internal = nullptr;
    }

    auto updateState(util::OMTicker &t) -> void
    {
        std::map<std::string, uint64_t> results;
        uint64_t total = 0;
        for (auto &e : t.ticks)
        {
            if (e.first.depth != 1)
            {
                continue;
            }

            if (e.first.pop)
            {
                results[e.first.id] = e.second - results[e.first.id];
                total += results[e.first.id];
            }
            else
            {
                results[e.first.id] = e.second;
            }
        }

        while (sectorNodes.size() != results.size())
        {
            if (sectorNodes.size() > results.size())
            {
                graphNode->umount(sectorNodes.back());
                textNode->umount(textNodes.back());
                sectorNodes.pop_back();
                textNodes.pop_back();
            }
            else
            {
                auto n = std::make_shared<node::OMDemiurgeSectorNode>()->style({
                    {"position", OMDemiurgePosition::Absolute},
                    {"width", 100_percent},
                    {"height", 100_percent},
                    {"rotationPivot", glm::vec3(0.0f, -2.0f, 1.0f)},
                });
                graphNode->mount(n);
                sectorNodes.emplace_back(n);

                auto n2 = std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                              ->style({
                                  {"flexGrow", 0.0f},
                                  {"textheight", 14},
                              });
                textNode->mount(n2);
                textNodes.emplace_back(n2);
            }
        }

        float curang = glm::radians(90.0f);
        int i = 0;
        std::vector<std::pair<std::string, uint64_t>> vec(results.begin(), results.end());

        std::sort(vec.begin(), vec.end(), [](const auto &a, const auto &b) -> auto { return a.second > b.second; });
        for (auto &p : vec)
        {
            auto c = DebugColors[(i + 3) % 16];
            sectorNodes[i]->style("beginAngle", curang)->style("color", c);
            auto fct = (double)p.second / (double)total;
            curang += glm::radians(360.0f) * fct;
            sectorNodes[i]->style("endAngle", curang);
            textNodes[i]
                ->style("text", fmt::format("{}: {:.2f} us / {:.2f} %", p.first, static_cast<float>(p.second) / 1000,
                                            fct * 100))
                ->style("color", c);
            ++i;
        }
    }

    void submitTasks() override
    {
    }
    void beforeFrame() override
    {
    }
    void afterFrame() override
    {
        ++fps;

        if (tp == std::chrono::steady_clock::time_point{})
        {
            tp = std::chrono::steady_clock::now();
            return;
        }

        auto tpe = std::chrono::steady_clock::now();
        auto cc = std::chrono::duration_cast<std::chrono::nanoseconds>(tpe - tp);
        if (cc.count() > 1e8)
        {
            fpsTextNode->style("text",
                               fmt::format("FPS: {}", static_cast<int>(static_cast<float>(fps) / cc.count() * 1e9)));

            tp = std::chrono::steady_clock::now();
            fps = 0;
        }

        auto m = camera->getPosRaw();
        posTextNode->style("text", fmt::format("{} {} {} + {:.2f} {:.2f} {:.2f}", m.chunkx, m.chunky, m.chunkz,
                                               m.localx, m.localy, m.localz));
        float fx = static_cast<float>(m.chunkx) * 16 + m.localx;
        double dx = static_cast<double>(m.chunkx) * 16 + m.localx;
        float fz = static_cast<float>(m.chunkz) * 16 + m.localz;
        double dz = static_cast<double>(m.chunkz) * 16 + m.localz;
        povTextNode->style("text", fmt::format("Yaw {:.2f} Pitch {:.2f}", camera->getYaw(), camera->getPitch()));
        precisionNode->style("text", fmt::format("float precision: {}", getUlpf(std::max(fx, fz))));
        precisionNode2->style("text", fmt::format("double precision: {}", getUlp(std::max(dx, dz))));
    }
    auto getUlpf(float s) -> float
    {
        if (s == 0.0)
        {
            return std::numeric_limits<float>::min();
        }

        int exp;
        std::frexpf(s, &exp);
        return std::ldexp(1.0, exp - 1 - 23);
    }
    auto getUlp(double s) -> double
    {
        if (s == 0.0)
        {
            return std::numeric_limits<double>::min();
        }

        int exp;
        std::frexp(s, &exp);
        return std::ldexp(1.0, exp - 1 - 52);
    }

    std::shared_ptr<OMDemiurgeNode> node, graphNode, textNode, fpsTextNode, posTextNode, povTextNode, precisionNode,
        precisionNode2;
    std::shared_ptr<OMDemiurgeRendererHandler> internal;
    std::shared_ptr<fontproc::OMFontSet> fontset;
    std::vector<std::shared_ptr<OMDemiurgeNode>> sectorNodes = {};
    std::vector<std::shared_ptr<OMDemiurgeNode>> textNodes = {};
    int fps = 0;
    std::chrono::steady_clock::time_point tp = {};
    OMRenderer *renderer;
    basics::OMCamera *camera;
};

bool isRunning = true;
void rendererTest(OMBackend backend)
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

        auto hnd2 = std::make_shared<OMNodeRendererHandler>(win());
        auto hnd = std::make_shared<renderer::OMTestRenderer>(
            win(), [&]() -> OMRendererTexture * { return hnd2->internal->middleTarget->colorTexture; }, bus, camera);
        hnd2->camera = camera.get();
        win()->registerHandler(hnd2);
        win()->registerHandler(hnd);
        win()->baseInit();

        std::array<bool, 6> keystates = {false, false, false, false, false, false};
        bus.append(SDL_EVENT_WINDOW_RESIZED, [&](SDL_Event &) -> void { win()->requestResize(); });
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
