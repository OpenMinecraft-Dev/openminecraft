#include "openminecraft-shell/renderer/om_debug_renderer.hpp"
#include <algorithm>
#include <fstream>
#include <functional>
#include "openminecraft-shell/data/block/om_blockstate_registry.hpp"
#include "openminecraft-shell/om_application.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <array>
#include <memory>
#include <string>

using namespace openminecraft::renderer;
using namespace openminecraft;
using namespace openminecraft::renderer::common::demiurge;
using namespace openminecraft::renderer::common;

namespace openminecraft::shell::renderer
{
static inline auto fromBytes(uint64_t l) -> std::string
{
    if (l < 1024)
    {
        return fmt::format("{} B", l);
    }
    if (l < 1024 * 1024)
    {
        return fmt::format("{:.2f} KB", static_cast<double>(l) / 1024);
    }
    if (l < 1024 * 1024 * 1024)
    {
        return fmt::format("{:.2f} MB", static_cast<double>(l) / 1024 / 1024);
    }
    if (l < 1024ull * 1024 * 1024 * 1024)
    {
        return fmt::format("{:.2f} GB", static_cast<double>(l) / 1024 / 1024 / 1024);
    }
    return fmt::format("{:.2f} TB", static_cast<double>(l) / 1024 / 1024 / 1024 / 1024);
}
OMDebugRenderer::OMDebugRenderer(OMRenderer *renderer, openminecraft::renderer::common::event::OMEventBusWrap &bus)
    : OMRendererHandler(renderer)
{
    this->renderer = renderer;

    fontset = std::make_shared<geom::OMFontSet>();

    auto rawfile2 = vfs::fsfetch("/bootassets/openminecraft-renderer/font/MapleMono-NF-Regular.ttf");
    fontset->fontList.push_back(std::make_shared<geom::OMFont>(*rawfile2.get()));

    auto button = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
    button->setOnClick([&]() {
        event::OMEvent e;
        e.type = event::Custom;
        e.custom.flag = EventMainScreenOpen;
        bus.handle(event::Custom, e);
    });
    button->setBackgroundColor({0.17, 0.17, 0.20});
    node = std::make_shared<node::OMDemiurgeContainerNode>()
               ->style({
                   {"flexDirection", Row},
                   {"justifyContent", OMDemiurgeAlign::SpaceBetween},
                   {"width", OMDemiurgeSize::fit()},
                   {"height", OMDemiurgeSize::fit()},
                   {"alignItems", OMDemiurgeAlign::FlexStart},
               })
               ->mount(std::make_shared<node::OMDemiurgeContainerNode>()
                           ->style({
                               {"flexDirection", Column},
                               {"width", OMDemiurgeSize::fit()},
                               {"height", OMDemiurgeSize::fit()},
                               {"alignItems", OMDemiurgeAlign::FlexStart},
                           })
                           ->mount(std::make_shared<node::OMDemiurgeRectNode>()
                                       ->style({
                                           {"color", (int)0x2c2c3433},
                                           {"flexDirection", Column},
                                           {"flexGap", 5_px},
                                           {"width", OMDemiurgeSize::fit()},
                                           {"height", OMDemiurgeSize::fit()},
                                           {"radius", glm::vec4(5.0f)},
                                           {"margin", std::array<OMDemiurgeSize, 4>{10_px, 10_px, 10_px, 10_px}},
                                           {"border", OMDemiurgeEdgeInsets{10, 10, 10, 10}},
                                       })
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0x00d4ffff},
                                                       {"text", fmt::format("OpenMinecraft {}-{}", OM_VERSION,
                                                                            OM_VERSION_CHANNEL)},
                                                       {"textheight", 16},
                                                   }))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", "\"virtual testing world\""},
                                                       {"textheight", 12},
                                                   }))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 12},
                                                   })
                                                   ->store(fpsTextNode))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 12},
                                                   })
                                                   ->store(posTextNode))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 12},
                                                   })
                                                   ->store(povTextNode))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 12},
                                                   })
                                                   ->store(tickNode)))
                           ->mount(std::make_shared<node::OMDemiurgeRectNode>()
                                       ->style({
                                           {"color", (int)0x2c2c3433},
                                           {"flexDirection", Column},
                                           {"flexGap", 5_px},
                                           {"width", OMDemiurgeSize::fit()},
                                           {"height", OMDemiurgeSize::fit()},
                                           {"radius", glm::vec4(5.0f)},
                                           {"margin", std::array<OMDemiurgeSize, 4>{10_px, 10_px, 10_px, 10_px}},
                                           {"border", OMDemiurgeEdgeInsets{10, 10, 10, 10}},
                                       })
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0x00d4ffff},
                                                       {"text", "Operations"},
                                                       {"textheight", 16},
                                                   }))
                                       ->mount(button->style("animation_speed", 1.0f)->style("label", "Main Menu"))))
               ->mount(std::make_shared<node::OMDemiurgeContainerNode>()
                           ->style({
                               {"flexDirection", Column},
                               {"width", OMDemiurgeSize::fit()},
                               {"height", OMDemiurgeSize::fit()},
                               {"alignItems", OMDemiurgeAlign::FlexEnd},
                           })
                           ->mount(std::make_shared<node::OMDemiurgeRectNode>()
                                       ->style({
                                           {"color", (int)0x2c2c3433},
                                           {"flexDirection", Column},
                                           {"flexGap", 5_px},
                                           {"width", OMDemiurgeSize::fit()},
                                           {"height", OMDemiurgeSize::fit()},
                                           {"radius", glm::vec4(5.0f)},
                                           {"margin", std::array<OMDemiurgeSize, 4>{10_px, 10_px, 10_px, 10_px}},
                                           {"border", OMDemiurgeEdgeInsets{10, 10, 10, 10}},
                                           {"alignItems", OMDemiurgeAlign::FlexEnd},
                                       })
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0x00d4ffff},
                                                       {"text", "Hardware/software stats"},
                                                       {"textheight", 16},
                                                   }))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", renderer->driver()},
                                                       {"textheight", 12},
                                                   }))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", vm::os::fetchCpuName()},
                                                       {"textheight", 12},
                                                   }))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text",
                                                        vm::os::fetchSystemName() + " " + vm::os::fetchSystemVersion()},
                                                       {"textheight", 12},
                                                   }))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", fromBytes(vm::os::fetchMemoryTotal()) + " / Page " +
                                                                    fromBytes(vm::os::fetchPageSize())},
                                                       {"textheight", 12},
                                                   })))
                           ->mount(std::make_shared<node::OMDemiurgeRectNode>()
                                       ->style({
                                           {"color", (int)0x2c2c3433},
                                           {"flexDirection", Column},
                                           {"flexGap", 5_px},
                                           {"width", OMDemiurgeSize::fit()},
                                           {"height", OMDemiurgeSize::fit()},
                                           {"radius", glm::vec4(5.0f)},
                                           {"margin", std::array<OMDemiurgeSize, 4>{10_px, 10_px, 10_px, 10_px}},
                                           {"border", OMDemiurgeEdgeInsets{10, 10, 10, 10}},
                                           {"alignItems", OMDemiurgeAlign::FlexEnd},
                                       })
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0x00d4ffff},
                                                       {"text", "Precision stats"},
                                                       {"textheight", 16},
                                                   }))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 12},
                                                   })
                                                   ->store(precisionNode))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 12},
                                                   })
                                                   ->store(precisionNode2)))
                           ->mount(std::make_shared<node::OMDemiurgeRectNode>()
                                       ->style({
                                           {"color", (int)0x2c2c3433},
                                           {"flexDirection", Column},
                                           {"flexGap", 5_px},
                                           {"width", OMDemiurgeSize::fit()},
                                           {"height", OMDemiurgeSize::fit()},
                                           {"radius", glm::vec4(5.0f)},
                                           {"margin", std::array<OMDemiurgeSize, 4>{10_px, 10_px, 10_px, 10_px}},
                                           {"border", OMDemiurgeEdgeInsets{10, 10, 10, 10}},
                                           {"alignItems", OMDemiurgeAlign::FlexEnd},
                                       })
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0x00d4ffff},
                                                       {"text", "Chunk stats"},
                                                       {"textheight", 16},
                                                   }))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 12},
                                                   })
                                                   ->store(vState))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 12},
                                                   })
                                                   ->store(vcState))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 12},
                                                   })
                                                   ->store(vfState))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 12},
                                                   })
                                                   ->store(vtState))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 12},
                                                   })
                                                   ->store(vtcState))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 12},
                                                   })
                                                   ->store(vtfState))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 12},
                                                   })
                                                   ->store(chunkstate))));

    internal = std::make_shared<OMDemiurgeRendererHandler>(renderer, node);
    internal->name = "debughud";
    renderer->registerHandler(internal);
}
OMDebugRenderer::~OMDebugRenderer()
{
    internal = nullptr;
}

void OMDebugRenderer::submitTasks()
{
}
void OMDebugRenderer::beforeFrame()
{
}
static std::ofstream off("test.csv");
void OMDebugRenderer::afterFrame()
{
    const float frameTime = renderer->fetchFrameTime();
    if (frameTime > 0.0f)
    {
        frameTimes.push_back(frameTime);

        if (frameTimes.size() > 600)
        {
            frameTimes.erase(frameTimes.begin(),
                             frameTimes.begin() + static_cast<std::ptrdiff_t>(frameTimes.size() - 600));
        }
    }

    if (!frameTimes.empty())
    {
        double sum = 0.0;

        for (auto v : frameTimes)
        {
            sum += v;
        }

        const double avgMs = sum / static_cast<double>(frameTimes.size());
        const double avgFps = avgMs > 0.0 ? 1000.0 / avgMs : 0.0;

        std::vector<float> sorted = frameTimes;
        std::sort(sorted.begin(), sorted.end());

        auto percentile = [&](double p) -> double {
            const size_t idx =
                std::min(sorted.size() - 1, static_cast<size_t>(p * static_cast<double>(sorted.size() - 1)));
            return 1000.0 / sorted[idx];
        };

        const size_t lowCount = std::max<size_t>(1, sorted.size() / 100);
        double lowSum = 0.0;

        for (size_t i = sorted.size() - lowCount; i < sorted.size(); ++i)
        {
            lowSum += sorted[i];
        }

        const double lowMs = lowSum / static_cast<double>(lowCount);
        const double lowFps = lowMs > 0.0 ? 1000.0 / lowMs : 0.0;
        const double instantFps = 1000.0 / frameTime;

        fpsTextNode->style(
            "text", fmt::format("FPS: {:.0f} (avg {:.0f}, 1% low {:.0f}, p50 {:.2f} p95 {:.2f} p99 {:.2f})", instantFps,
                                avgFps, lowFps, percentile(0.50), percentile(0.95), percentile(0.99)));

        auto fo = fmt::format("{:.0f},{:.0f},{:.0f},{:.0f},{:.0f},{:.0f}\n", instantFps, avgFps, lowFps,
                              percentile(0.50), percentile(0.95), percentile(0.99));
        off.write(fo.data(), fo.size());
        off.flush();
    }

    auto m = camera->getPosRaw();
    posTextNode->style("text", fmt::format("{} {} {} + {:.2f} {:.2f} {:.2f}", m.chunkx, m.chunky, m.chunkz, m.localx,
                                           m.localy, m.localz));
    float fx = std::abs(static_cast<float>(m.chunkx) * 16 + m.localx);
    double dx = std::abs(static_cast<double>(m.chunkx) * 16 + m.localx);
    float fz = std::abs(static_cast<float>(m.chunkz) * 16 + m.localz);
    double dz = std::abs(static_cast<double>(m.chunkz) * 16 + m.localz);
    povTextNode->style("text", fmt::format("Yaw {:.2f} Pitch {:.2f}", camera->getYaw(), camera->getPitch()));
    precisionNode->style("text", fmt::format("float: {}", getUlpf(std::max(fx, fz))));
    precisionNode2->style("text", fmt::format("double: {}", getUlp(std::max(dx, dz))));

    auto st = stateFetch();
    auto lst = std::vector{vState, vcState, vfState, vtState, vtcState, vtfState};
    auto tags =
        std::vector{"Voxel", "VoxelComplex", "VoxelFluid", "VoxelTrans", "VoxelTransComplex", "VoxelTransFluid"};

    for (int i = 0; i < 6; ++i)
    {
        const auto &d = st[i];
        lst[i]->style("text", fmt::format("{}: {} / {}, {} blocks", tags[i], fromBytes(std::get<0>(d)),
                                          fromBytes(std::get<1>(d)), std::get<2>(d)));
    }

    auto chunkst = chunkFetch();
    chunkstate->style("text", fmt::format("{} chunks / {} rendered", chunkst.first, chunkst.second));

    tickNode->style("text", fmt::format("Tick: {}", gameTickFetch()));
}
} // namespace openminecraft::shell::renderer
