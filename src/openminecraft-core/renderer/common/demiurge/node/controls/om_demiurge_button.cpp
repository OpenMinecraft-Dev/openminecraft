#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "glm/ext/vector_float3.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_cliprect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include <array>
#include <SDL3/SDL.h>
#include <memory>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
OMDemiurgeButton::OMDemiurgeButton(geom::OMFontSet *fontset)
    : opacity(0.6f), backgroundColor({0.1f, 0.1f, 0.1f}), textColor({1.0f, 1.0f, 1.0f, 1.0f})
{
    stylesStorage.put("justifyContent", OMDemiurgeAlign::Center);
    stylesStorage.put("alignItems", OMDemiurgeAlign::Center);
    bkgNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"position", Absolute},
        {"width", 100_percent},
        {"height", 100_percent},
    });
    textNode = std::make_shared<OMDemiurgeTextSdfNode>(fontset)->style({
        {"alignSelf", OMDemiurgeAlign::Center},
        {"margin", std::array<OMDemiurgeSize, 4>{5_px, 10_px, 5_px, 5_px}},
    });
    this->mountDirect(bkgNode);
    this->mountDirect(textNode);
}
OMDemiurgeButton::~OMDemiurgeButton() = default;
void OMDemiurgeButton::update()
{
    bkgNode->style("color", (int)(f3ToRgba(backgroundColor.get()) | static_cast<uint8_t>(opacity.get() * 255.0f)));
    bkgNode->style("radius", stylesStorage.get<glm::vec4>("radius", glm::vec4(5.0f)));
    textNode->style("color", f4ToRgba(textColor.get()));
    textNode->style("text", stylesStorage.get<std::string>("label", "Button"));
    textNode->style("textheight", stylesStorage.get<int>("textheight", 16));
    OMDemiurgeContainerNode::update();
}
auto OMDemiurgeButton::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    opacity.animateTo(stylesStorage.get<float>("opacity_clicked", 1.0f), easeOutCirc<float>,
                      stylesStorage.get<float>("animation_speed", 0.2f));
    return Handled;
}
auto OMDemiurgeButton::processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    opacity.animateTo(stylesStorage.get<float>("opacity_hovered", 0.8f), easeOutCirc<float>,
                      stylesStorage.get<float>("animation_speed", 0.2f));
    return Handled;
}

auto OMDemiurgeButton::processMouseEnter(float x, float y) -> OMDemiurgeEventResult
{
    opacity.animateTo(stylesStorage.get<float>("opacity_hovered", 0.8f), easeOutCirc<float>,
                      stylesStorage.get<float>("animation_speed", 0.2f));
    return Handled;
}
auto OMDemiurgeButton::processMouseExit(float x, float y) -> OMDemiurgeEventResult
{
    opacity.animateTo(stylesStorage.get<float>("opacity_normal", 0.6f), easeOutCirc<float>,
                      stylesStorage.get<float>("animation_speed", 0.2f));
    return Handled;
}

auto OMDemiurgeButton::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    OMDemiurgeContainerNode::submit(handler, depth);
}

void OMDemiurgeButton::setText(std::string s)
{
    textNode->style("text", s);
}
void OMDemiurgeButton::setTextColor(glm::vec4 d)
{
    textColor.animateTo(d, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.2f));
}
void OMDemiurgeButton::setBackgroundColor(glm::vec3 d)
{
    backgroundColor.animateTo(d, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.2f));
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
