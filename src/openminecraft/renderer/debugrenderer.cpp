#include "openminecraft-shell/renderer/debugrendrer.hpp"
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

namespace openminecraftshell::renderer
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
OMDebugRenderer::OMDebugRenderer(OMRenderer *renderer) : OMRendererHandler(renderer)
{
    this->renderer = renderer;

    fontset = std::make_shared<geom::OMFontSet>();

    auto rawfile2 = vfs::fsfetch("/bootassets/openminecraft-boot/font/MapleMono-NF-Regular.ttf");
    fontset->fontList.push_back(std::make_shared<geom::OMFont>(*rawfile2.get()));
    auto rawfile1 = vfs::fsfetch("/bootassets/openminecraft-boot/font/StarRailFont.ttf");
    fontset->fontList.push_back(std::make_shared<geom::OMFont>(*rawfile1.get()));

    auto button = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
    button->setOnClick([]() { exit(0); });
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
                                                       {"textheight", 16},
                                                   }))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 16},
                                                   })
                                                   ->store(fpsTextNode))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 16},
                                                   })
                                                   ->store(posTextNode))
                                       ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                                   ->style({
                                                       {"color", (int)0xffffffff},
                                                       {"text", ""},
                                                       {"textheight", 16},
                                                   })
                                                   ->store(povTextNode)))
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
                                       ->mount(button->style("animation_speed", 1.0f)->style("label", "Quit"))))
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
                                           {"text", "Hardware/software & lowlevel stats"},
                                           {"textheight", 16},
                                       }))
                           ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                       ->style({
                                           {"color", (int)0xffffffff},
                                           {"text", renderer->driver()},
                                           {"textheight", 16},
                                       }))
                           ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                       ->style({
                                           {"color", (int)0xffffffff},
                                           {"text", vm::os::fetchCpuName()},
                                           {"textheight", 16},
                                       }))
                           ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                       ->style({
                                           {"color", (int)0xffffffff},
                                           {"text", vm::os::fetchSystemName() + " " + vm::os::fetchSystemVersion()},
                                           {"textheight", 16},
                                       }))
                           ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                       ->style({
                                           {"color", (int)0xffffffff},
                                           {"text", fromBytes(vm::os::fetchMemoryTotal()) + " / Page " +
                                                        fromBytes(vm::os::fetchPageSize())},
                                           {"textheight", 16},
                                       }))
                           ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                       ->style({
                                           {"color", (int)0xffffffff},
                                           {"text", ""},
                                           {"textheight", 16},
                                       })
                                       ->store(precisionNode))
                           ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                       ->style({
                                           {"color", (int)0xffffffff},
                                           {"text", ""},
                                           {"textheight", 16},
                                       })
                                       ->store(precisionNode2)));

    internal = std::make_shared<OMDemiurgeRendererHandler>(renderer, node);
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
int i = 0;
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
    if (cc.count() > 5e8)
    {
        fpsTextNode->style("text",
                           fmt::format("FPS: {}", static_cast<int>(static_cast<float>(fps) / cc.count() * 1e9)));

        tp = std::chrono::steady_clock::now();
        fps = 0;
    }

    auto m = camera->getPosRaw();
    posTextNode->style("text", fmt::format("{} {} {} + {:.2f} {:.2f} {:.2f}", m.chunkx, m.chunky, m.chunkz, m.localx,
                                           m.localy, m.localz));
    float fx = std::abs(static_cast<float>(m.chunkx) * 16 + m.localx);
    double dx = std::abs(static_cast<double>(m.chunkx) * 16 + m.localx);
    float fz = std::abs(static_cast<float>(m.chunkz) * 16 + m.localz);
    double dz = std::abs(static_cast<double>(m.chunkz) * 16 + m.localz);
    povTextNode->style("text", fmt::format("Yaw {:.2f} Pitch {:.2f}", camera->getYaw(), camera->getPitch()));
    precisionNode->style("text", fmt::format("float precision: {}", getUlpf(std::max(fx, fz))));
    precisionNode2->style("text", fmt::format("double precision: {}", getUlp(std::max(dx, dz))));
}
} // namespace openminecraftshell::renderer
