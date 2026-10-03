#include "openminecraft-shell/renderer/surfacerenderer.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_styles.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <array>
#include <iostream>
#include <memory>

using namespace openminecraft::renderer;
using namespace openminecraft;
using namespace openminecraft::renderer::common::demiurge;

namespace openminecraftshell::renderer
{
OMSurfaceRenderer::OMSurfaceRenderer(openminecraft::renderer::OMRenderer *renderer)
    : OMRendererHandler(renderer), renderer(renderer), offset(0.0)
{
    fontset = std::make_shared<geom::OMFontSet>();

    auto rawfile1 = vfs::fsfetch("/bootassets/openminecraft-boot/font/StarRailFont.ttf");
    fontset->fontList.push_back(std::make_shared<geom::OMFont>(*rawfile1.get()));

    auto button = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
    button->setOnClick([&]() { offset.animateTo(1.0, common::animation::easeInCubic<float>, 1.0); });
    button->setBackgroundColor({0.17, 0.17, 0.20});

    node = std::make_shared<node::OMDemiurgeRectNode>()
               ->style({
                   {"color", (int)0x2c2c3433},
                   {"flexDirection", Column},
                   {"justifyContent", SpaceBetween},
                   {"width", OMDemiurgeSize::fit()},
                   {"height", OMDemiurgeSize::fit()},
                   {"radius", glm::vec4(5.0f)},
                   {"margin", std::array<OMDemiurgeSize, 4>{10_px, 10_px, 10_px, 10_px}},
                   {"border", OMDemiurgeEdgeInsets{50, 10, 100, 10}},
               })
               ->mount(std::make_shared<node::OMDemiurgeContainerNode>()
                           ->style({
                               {"flexDirection", Column},
                               {"flexGap", 40_px},
                               {"border", OMDemiurgeEdgeInsets{10, 10, 10, 220}},
                           })
                           ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                       ->style({
                                           {"color", (int)0xffffffff},
                                           {"text", "OpenMinecraft"},
                                           {"textheight", 24},
                                           {"alignSelf", OMDemiurgeAlign::FlexStart},
                                       }))
                           ->mount(button->style("animation_speed", 1.0f)->style("label", "Demo Test")))
               ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                           ->style({
                               {"color", (int)0xffffffff},
                               {"text", fmt::format("Version {}-{}", OM_VERSION, OM_VERSION_CHANNEL)},
                               {"textheight", 12},
                               {"alignSelf", OMDemiurgeAlign::FlexEnd},
                           }));

    internal = std::make_shared<OMDemiurgeRendererHandler>(renderer, node);
    internal->name = "surface";
    renderer->registerHandler(internal);
}
OMSurfaceRenderer::~OMSurfaceRenderer()
{
}

void OMSurfaceRenderer::submitTasks()
{
}
void OMSurfaceRenderer::beforeFrame()
{
}
void OMSurfaceRenderer::afterFrame()
{
    node->style("offsetY", OMDemiurgeSize::percent(offset.get()));
}

void OMSurfaceRenderer::openScreen()
{
    offset.animateTo(0.0, common::animation::easeOutCubic<float>, 1.0);
}
} // namespace openminecraftshell::renderer