#include "openminecraft/boot/entrypoint_testrenderer.hpp"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_image.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_sector.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/om_renderer_window.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/util/om_util_ticker.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <array>
#include <chrono>
#include <memory>
#include <map>

using namespace openminecraft::renderer;
using namespace openminecraft::renderer::common::demiurge;

namespace openminecraft::boot
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
        {
            auto imgraw = vfs::fsfetch("/bootassets/openminecraft-renderer/texture/summer_1am.png");

            specs::png::OMPngFile img;
            img.parse(imgraw);

            texture = renderer->allocateTexture(img.getWidth(), img.getHeight(), Dim2, ColorRgba);
            texture->updateData(img.fetchData());
        }

        fontset = std::make_shared<fontproc::OMFontSet>();
        auto rawfile = vfs::fsfetch("/bootassets/openminecraft-boot/font/MapleMono-NF-Regular.ttf");
        fontset->fontList.push_back(std::make_shared<fontproc::OMFont>(*rawfile.get()));
        auto rawfile2 = vfs::fsfetch("/bootassets/openminecraft-boot/font/StarRailFont.ttf");
        fontset->fontList.push_back(std::make_shared<fontproc::OMFont>(*rawfile2.get()));
        auto rawfile3 = vfs::fsfetch("/bootassets/openminecraft-boot/font/NotoSansArabic.ttf");
        fontset->fontList.push_back(std::make_shared<fontproc::OMFont>(*rawfile3.get()));
        node = std::make_shared<node::OMDemiurgeImageNode>(texture)
                   ->style({
                       {"color", (int)0xffffff00},
                       {"flexGap", 10_px},
                       {"flexWrap", Wrap},
                       {"flexDirection", Column},
                       {"fill", node::OMDemiurgeImageFillType::Cover},
                       {"radius", glm::vec4(25.0f)},
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
                                               {"text", "OpenMinecraft Debug Screen"},
                                               {"textheight", 24},
                                           }))
                               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                           ->style({
                                               {"color", (int)0xffffffff},
                                               {"flexGrow", 1.0f},
                                               {"text", ""},
                                               {"textheight", 24},
                                           })
                                           ->store(fpsTextNode)))
                   ->mount(std::make_shared<node::OMDemiurgeRectNode>()
                               ->style({
                                   {"flexShrink", 0.0f},
                                   {"flexGrow", 1.0f},
                                   {"width", 100_percent},
                                   {"color", 0x22222200},
                                   {"radius", glm::vec4(25.0f)},
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
        delete texture;
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
                                  {"textheight", 24},
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
            sectorNodes[i]->style("beginAngle", curang)->style("color", DebugColors[i % 16]);
            auto fct = (double)p.second / (double)total;
            curang += glm::radians(360.0f) * fct;
            sectorNodes[i]->style("endAngle", curang);
            textNodes[i]
                ->style("text", fmt::format("{}: {:.2f} us / {:.2f} %", p.first, static_cast<float>(p.second) / 1000,
                                            fct * 100))
                ->style("color", DebugColors[i % 16]);
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
    }

    std::shared_ptr<OMDemiurgeNode> node, graphNode, textNode, fpsTextNode;
    OMRendererTexture *texture;
    std::shared_ptr<OMDemiurgeRendererHandler> internal;
    std::shared_ptr<fontproc::OMFontSet> fontset;
    std::vector<std::shared_ptr<OMDemiurgeNode>> sectorNodes = {};
    std::vector<std::shared_ptr<OMDemiurgeNode>> textNodes = {};
    int fps = 0;
    std::chrono::steady_clock::time_point tp = {};
};

void rendererTest(renderer::OMBackend backend)
{
    auto logger = std::make_shared<log::OMLogger>("Test Renderer");
    try
    {
        OMWindowConfig conf = {backend, false};
        // INFO: requires at least OpenGL 4.3 Core Profile or Vulkan 1.2
        OMWindow win({util::Version(4, 3, 0, 0), util::Version(1, 2, 0, 0)}, conf,
                     "/bootassets/openminecraft-renderer/shaders");
        auto hnd2 = std::make_shared<OMNodeRendererHandler>(win());
        auto hnd = std::make_shared<test::OMTestRenderer>(
            win(), [&]() -> OMRendererTexture * { return hnd2->internal->middleTexture; });
        win()->registerHandler(hnd2);
        win()->registerHandler(hnd);
        win()->baseInit();

        logger->info("driver: {}", win()->driver());

        // INFO: states[0] represents application status, states[1] represents if the window resized
        std::array<bool, 2> states = {false, false};
        util::OMTicker ticker;
        while (!states[0])
        {
            ticker.begin();

            hnd->eventLoop(*win, states.data());
            if (states[1])
            {
                win()->requestResize();
                states[1] = false;
            }

            win()->render(ticker);

            hnd2->updateState(ticker);
        }

        hnd = nullptr;
        hnd2 = nullptr;
    }
    catch (std::runtime_error &e)
    {
        logger->fatal(e.what());
    }
}
} // namespace openminecraft::boot
