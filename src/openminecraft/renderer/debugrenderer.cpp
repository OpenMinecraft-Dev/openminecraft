#include "openminecraft-shell/renderer/debugrendrer.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_image.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_sector.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <map>

using namespace openminecraft::renderer;
using namespace openminecraft;
using namespace openminecraft::renderer::common::demiurge;

namespace openminecraftshell::renderer
{
OMDebugRenderer::OMDebugRenderer(OMRenderer *renderer) : OMRendererHandler(renderer)
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
OMDebugRenderer::~OMDebugRenderer()
{
    internal = nullptr;
}

auto OMDebugRenderer::updateState(util::OMTicker &t) -> void
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
            ->style("text",
                    fmt::format("{}: {:.2f} us / {:.2f} %", p.first, static_cast<float>(p.second) / 1000, fct * 100))
            ->style("color", c);
        ++i;
    }
}

void OMDebugRenderer::submitTasks()
{
}
void OMDebugRenderer::beforeFrame()
{
}
void OMDebugRenderer::afterFrame()
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
    posTextNode->style("text", fmt::format("{} {} {} + {:.2f} {:.2f} {:.2f}", m.chunkx, m.chunky, m.chunkz, m.localx,
                                           m.localy, m.localz));
    float fx = static_cast<float>(m.chunkx) * 16 + m.localx;
    double dx = static_cast<double>(m.chunkx) * 16 + m.localx;
    float fz = static_cast<float>(m.chunkz) * 16 + m.localz;
    double dz = static_cast<double>(m.chunkz) * 16 + m.localz;
    povTextNode->style("text", fmt::format("Yaw {:.2f} Pitch {:.2f}", camera->getYaw(), camera->getPitch()));
    precisionNode->style("text", fmt::format("float precision: {}", getUlpf(std::max(fx, fz))));
    precisionNode2->style("text", fmt::format("double precision: {}", getUlp(std::max(dx, dz))));
}
} // namespace openminecraftshell::renderer