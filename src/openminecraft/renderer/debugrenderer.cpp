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

    auto rawfile2 = vfs::fsfetch("/bootassets/openminecraft-boot/font/MapleMono-NF-Regular.ttf");
    fontset->fontList.push_back(std::make_shared<fontproc::OMFont>(*rawfile2.get()));

    node = std::make_shared<node::OMDemiurgeRectNode>()
               ->style({
                   {"color", (int)0x23232399},
                   {"flexGap", 10_px},
                   {"flexWrap", Wrap},
                   {"flexDirection", Column},
                   {"fill", node::OMDemiurgeImageFillType::Cover},
                   {"radius", glm::vec4(0.0f, 0.0f, 0.0f, 25.0f)},
                   {"width", OMDemiurgeSize::fit()},
                   {"height", OMDemiurgeSize::fit()},
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
                                           {"textheight", 92},
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
                                       ->store(precisionNode2)))
               ->mount(std::make_shared<node::OMDemiurgeContainerNode>()
                           ->style({
                               {"flexShrink", 0.0f},
                               {"flexGrow", 1.0f},
                               {"width", 100_percent},
                           })
                           ->store(textNode));

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
    float fx = std::abs(static_cast<float>(m.chunkx) * 16 + m.localx);
    double dx = std::abs(static_cast<double>(m.chunkx) * 16 + m.localx);
    float fz = std::abs(static_cast<float>(m.chunkz) * 16 + m.localz);
    double dz = std::abs(static_cast<double>(m.chunkz) * 16 + m.localz);
    povTextNode->style("text", fmt::format("Yaw {:.2f} Pitch {:.2f}", camera->getYaw(), camera->getPitch()));
    precisionNode->style("text", fmt::format("float precision: {}", getUlpf(std::max(fx, fz))));
    precisionNode2->style("text", fmt::format("double precision: {}", getUlp(std::max(dx, dz))));
}
} // namespace openminecraftshell::renderer
