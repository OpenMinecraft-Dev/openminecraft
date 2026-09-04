#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <array>
#include <SDL3/SDL.h>
#include <memory>

namespace openminecraft::renderer::common::demiurge::node::controls
{
OMDemiurgeButton::OMDemiurgeButton(fontproc::OMFontSet *fontset)
{
    stylesStorage.put("justifyContent", OMDemiurgeAlign::Center);
    stylesStorage.put("alignItems", OMDemiurgeAlign::Center);
    bkgNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"color", 0x23232388},
        {"radius", glm::vec4(50.0f)},
        {"position", Absolute},
        {"width", 100_percent},
        {"height", 100_percent},
    });
    textNode = std::make_shared<OMDemiurgeTextSdfNode>(fontset)->style({
        {"text", "Button"},
        {"textheight", 16},
        {"color", 0xffffffff},
        {"alignSelf", OMDemiurgeAlign::Center},
        {"margin", std::array<OMDemiurgeSize, 4>{5_px, 10_px, 5_px, 5_px}},
    });
    this->mountDirect(bkgNode);
    this->mountDirect(textNode);
}
OMDemiurgeButton::~OMDemiurgeButton() = default;

auto OMDemiurgeButton::processEvent(float x, float y, OMDemiurgeEventType type, uint8_t ext, void *data)
    -> OMDemiurgeEventResult
{
    if (type == MouseDown)
    {
        handle();
        return Handled;
    }
    else
    {
        return Ignored;
    }
}

auto OMDemiurgeButton::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    OMDemiurgeContainerNode::submit(handler, depth);
}

void OMDemiurgeButton::setText(std::string s)
{
    textNode->style("text", s);
}
void OMDemiurgeButton::setTextColor(int c)
{
    textNode->style("color", c);
}
void OMDemiurgeButton::setBackgroundColor(int c)
{
    bkgNode->style("color", c);
}
void OMDemiurgeButton::setBackgroundRadius(glm::vec4 r)
{
    bkgNode->style("radius", r);
}
void OMDemiurgeButton::setOnClick(std::function<void()> h)
{
    handle = h;
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
