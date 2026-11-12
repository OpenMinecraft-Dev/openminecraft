#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "glm/ext/vector_float3.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_pane.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include <algorithm>
#include <array>
#include <SDL3/SDL.h>
#include <memory>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
OMDemiurgeButton::OMDemiurgeButton(geom::OMFontSet *fontset)
    : opacity(0.6f), backgroundColor({0.1f, 0.1f, 0.1f, 1.0f}), textColor({1.0f, 1.0f, 1.0f, 1.0f}),
      disabledAmount(0.0f)
{
    setFocusable(true);
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
    const float disabledValue = std::clamp(disabledAmount.get(), 0.0f, 1.0f);

    const glm::vec4 background = backgroundColor.get();
    const glm::vec3 backgroundRgb = glm::mix(glm::vec3(background), glm::vec3(0.30f), disabledValue * 0.75f);
    const float alpha =
        std::clamp(background.a * opacity.get() * (1.0f - disabledValue * 0.5f), 0.0f, 1.0f);

    glm::vec4 text = textColor.get();
    glm::vec3 textRgb = glm::mix(glm::vec3(text), glm::vec3(0.55f), disabledValue * 0.75f);

    bkgNode->style("color", (int)(f3ToRgba(backgroundRgb) | static_cast<uint8_t>(alpha * 255.0f)));
    bkgNode->style("radius", stylesStorage.get<glm::vec4>("radius", glm::vec4(5.0f)));
    textNode->style("color", (int)f4ToRgba(glm::vec4(textRgb, text.a)));
    textNode->style("text", stylesStorage.get<std::string>("label", "Button"));
    textNode->style("textheight", stylesStorage.get<int>("textheight", 16));
    textNode->style("margin", stylesStorage.get<std::array<OMDemiurgeSize, 4>>(
                                  "text_margin", std::array<OMDemiurgeSize, 4>{5_px, 10_px, 5_px, 5_px}));
    OMDemiurgePane::update();
}
auto OMDemiurgeButton::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    captureMouse();
    opacity.animateTo(stylesStorage.get<float>("opacity_clicked", 1.0f), easeOutCirc<float>,
                      stylesStorage.get<float>("animation_speed", 0.2f));
    return Handled;
}
auto OMDemiurgeButton::processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    const bool inside = containsPoint(x, y);
    opacity.animateTo(inside ? stylesStorage.get<float>("opacity_hovered", 0.8f)
                             : stylesStorage.get<float>("opacity_normal", 0.6f),
                      easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.2f));

    if (inside)
    {
        handle();
    }

    return Handled;
}

auto OMDemiurgeButton::processMouseEnter(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    opacity.animateTo(stylesStorage.get<float>("opacity_hovered", 0.8f), easeOutCirc<float>,
                      stylesStorage.get<float>("animation_speed", 0.2f));
    return Handled;
}
auto OMDemiurgeButton::processMouseExit(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    opacity.animateTo(stylesStorage.get<float>("opacity_normal", 0.6f), easeOutCirc<float>,
                      stylesStorage.get<float>("animation_speed", 0.2f));
    return Handled;
}

auto OMDemiurgeButton::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    OMDemiurgePane::submit(handler, depth);
}

void OMDemiurgeButton::setTextColor(glm::vec4 d)
{
    textColor.animateTo(d, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.2f));
}
void OMDemiurgeButton::setBackgroundColor(glm::vec4 d)
{
    backgroundColor.animateTo(d, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.2f));
}

void OMDemiurgeButton::setOpacity(float value)
{
    opacity.animateTo(std::clamp(value, 0.0f, 1.0f), easeOutCirc<float>,
                      stylesStorage.get<float>("animation_speed", 0.2f));
}
void OMDemiurgeButton::setBackgroundRadius(glm::vec4 r)
{
    bkgNode->style("radius", r);
}
void OMDemiurgeButton::setDisabled(bool value, bool animate)
{
    if (disabled == value)
    {
        return;
    }

    disabled = value;

    if (animate)
    {
        disabledAmount.animateTo(disabled ? 1.0f : 0.0f, easeOutCirc<float>,
                                 stylesStorage.get<float>("animation_speed", 0.2f));
    }
    else
    {
        disabledAmount.setTo(disabled ? 1.0f : 0.0f);
    }
}

void OMDemiurgeButton::setOnClick(std::function<void()> h)
{
    handle = h;
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
