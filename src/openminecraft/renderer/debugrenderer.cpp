#include "openminecraft-shell/renderer/debugrendrer.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_cliprect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_svg.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <array>
#include <iostream>
#include <memory>
#include <string>

using namespace openminecraft::renderer;
using namespace openminecraft;
using namespace openminecraft::renderer::common::demiurge;

namespace openminecraftshell::renderer
{
const std::array<std::string, 8> loadingRing = {
    "M2.757 6.046c-.263.123-.38.439-.214.676a3 3 0 1 0 .012-3.46c-.168.235-.053.552.21.676.261.125.57.007.76-.213a1.95 "
    "1.95 0 1 1-.01 2.54c-.188-.221-.495-.341-.758-.219",
    "M2.563 4.57C2.277 4.52 2 4.711 2 5.001a3 3 0 1 0 "
    "1.995-2.828c-.273.098-.36.423-.218.675.144.252.464.332.745.261A1.95 1.95 0 1 1 3.06 "
    "5.184c-.028-.288-.21-.563-.496-.614",
    "M3.954 2.757c-.123-.263-.439-.38-.676-.214a3 3 0 1 0 "
    "3.46.012c-.235-.168-.552-.053-.676.21-.125.261-.007.57.213.76a1.95 1.95 0 1 "
    "1-2.54-.01c.221-.187.341-.495.219-.758",
    "M5.846 2.674c.1-.272-.041-.578-.327-.629a3 3 0 1 0 2.44 "
    "2.456c-.05-.286-.354-.429-.627-.331-.274.097-.408.399-.387.688a1.95 1.95 0 1 "
    "1-1.79-1.802c.29.023.592-.11.691-.382",
    "M7.243 3.954c.263-.123.38-.438.214-.676a3 3 0 1 0-.012 "
    "3.461c.168-.236.053-.553-.21-.677-.261-.125-.57-.007-.76.213a1.95 1.95 0 1 1 .01-2.54c.187.221.495.342.758.219",
    "M7.326 5.846c.272.1.578-.04.629-.327a3 3 0 1 0-2.456 "
    "2.44c.286-.049.428-.354.331-.627-.097-.274-.399-.408-.688-.387a1.95 1.95 0 1 1 1.802-1.79c-.023.29.11.592.382.691",
    "M6.046 7.243c.122.263.438.38.676.214a3 3 0 1 0-3.461-.012c.236.168.553.053.677-.21.125-.261.006-.57-.213-.76a1.95 "
    "1.95 0 1 1 2.54.01c-.221.187-.342.495-.22.758",
    "M4.153 7.326c-.099.272.042.578.327.629a3 3 0 1 0-2.438-2.456c.048.286.353.428.626.331s.408-.399.387-.688a1.95 "
    "1.95 0 1 1 1.79 1.802c-.29-.023-.592.11-.692.382",
};
OMDebugRenderer::OMDebugRenderer(OMRenderer *renderer) : OMRendererHandler(renderer)
{
    this->renderer = renderer;

    fontset = std::make_shared<geom::OMFontSet>();

    auto rawfile2 = vfs::fsfetch("/bootassets/openminecraft-boot/font/MapleMono-NF-Regular.ttf");
    fontset->fontList.push_back(std::make_shared<geom::OMFont>(*rawfile2.get()));
    auto rawfile1 = vfs::fsfetch("/bootassets/openminecraft-boot/font/StarRailFont.ttf");
    fontset->fontList.push_back(std::make_shared<geom::OMFont>(*rawfile1.get()));

    auto button = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
    button->setOnClick([]() -> void { std::cout << "button 1 clicked!" << std::endl; });
    auto button2 = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
    button2->setOnClick([]() -> void { std::cout << "button 2 clicked!" << std::endl; });
    node = std::make_shared<node::OMDemiurgeContainerNode>()
               ->style({
                   {"flexDirection", Column},
                   {"flexGap", 5_px},
                   {"width", OMDemiurgeSize::fit()},
                   {"height", OMDemiurgeSize::fit()},
               })
               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                           ->style({
                               {"color", (int)0xffffffff},
                               {"flexGrow", 1.0f},
                               {"text", "OpenMinecraft Demo"},
                               {"textheight", 16},
                           }))
               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                           ->style({
                               {"color", (int)0xffffffff},
                               {"flexGrow", 1.0f},
                               {"text", ""},
                               {"textheight", 16},
                           })
                           ->store(fpsTextNode))
               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                           ->style({
                               {"color", (int)0xffffffff},
                               {"flexGrow", 1.0f},
                               {"text", ""},
                               {"textheight", 16},
                           })
                           ->store(posTextNode))
               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                           ->style({
                               {"color", (int)0xffffffff},
                               {"flexGrow", 1.0f},
                               {"text", ""},
                               {"textheight", 16},
                           })
                           ->store(povTextNode))
               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                           ->style({
                               {"color", (int)0xffffffff},
                               {"flexGrow", 1.0f},
                               {"text", renderer->driver()},
                               {"textheight", 16},
                           }))
               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                           ->style({
                               {"color", (int)0xffffffff},
                               {"flexGrow", 1.0f},
                               {"text", ""},
                               {"textheight", 16},
                           })
                           ->store(precisionNode))
               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                           ->style({
                               {"color", (int)0xffffffff},
                               {"flexGrow", 1.0f},
                               {"text", ""},
                               {"textheight", 16},
                           })
                           ->store(precisionNode2))
               ->mount(button->style("animation_speed", 1.0f))
               ->mount(button2
                           ->mount(std::make_shared<node::OMDemiurgeSvgNode>()
                                       ->style({
                                           {"width", 20_px},
                                           {"height", 20_px},
                                           {"color", (int)0xffffffff},
                                           {"svgPath", loadingRing[0]},
                                       })
                                       ->store(svgNode))
                           ->style("label", ""));

    internal = std::make_shared<OMDemiurgeRendererHandler>(renderer, node);
    internal->fit = true;
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
        svgNode->style("svgPath", loadingRing[(++i) % 8]);
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
