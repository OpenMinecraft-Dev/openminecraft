#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_iconbutton.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_svg.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

#include <algorithm>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
OMDemiurgeIconButton::OMDemiurgeIconButton(float size)
    : hoverAmount(0.0f), pressAmount(0.0f), disabledAmount(0.0f),
      iconColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)), backgroundColor(glm::vec4(0.17f, 0.17f, 0.20f, 0.6f)),
      buttonSize(size)
{
    setFocusable(true);

    stylesStorage.put("width", OMDemiurgeSize::pixels(buttonSize));
    stylesStorage.put("height", OMDemiurgeSize::pixels(buttonSize));

    backgroundNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"position", OMDemiurgePosition::Absolute},
        {"offsetX", OMDemiurgeSize::pixels(0.0f)},
        {"offsetY", OMDemiurgeSize::pixels(0.0f)},
        {"width", OMDemiurgeSize::fill()},
        {"height", OMDemiurgeSize::fill()},
        {"radius", glm::vec4(6.0f)},
    });

    const float inset = buttonSize * 0.25f;

    iconNode = std::make_shared<OMDemiurgeSvgNode>()->style({
        {"position", OMDemiurgePosition::Absolute},
        {"offsetX", OMDemiurgeSize::pixels(inset)},
        {"offsetY", OMDemiurgeSize::pixels(inset)},
        {"width", OMDemiurgeSize::pixels(buttonSize - inset * 2.0f)},
        {"height", OMDemiurgeSize::pixels(buttonSize - inset * 2.0f)},
        {"svgSize", glm::vec2(14.0f, 14.0f)},
        {"factor", 0.08f},
    });

    this->mountDirect(backgroundNode);
    this->mountDirect(iconNode);
}

OMDemiurgeIconButton::~OMDemiurgeIconButton() = default;

auto OMDemiurgeIconButton::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    OMDemiurgeContainerNode::submit(handler, depth);
}

void OMDemiurgeIconButton::update()
{
    const float hover = std::clamp(hoverAmount.get(), 0.0f, 1.0f);
    const float press = std::clamp(pressAmount.get(), 0.0f, 1.0f);
    const float disabledValue = std::clamp(disabledAmount.get(), 0.0f, 1.0f);

    glm::vec4 background = backgroundColor.get();
    background = glm::vec4(glm::mix(glm::vec3(background), glm::vec3(1.0f), hover * 0.12f),
                           background.a * (1.0f - press * 0.12f));
    background = glm::vec4(glm::mix(glm::vec3(background), glm::vec3(0.30f), disabledValue * 0.75f),
                           background.a * (1.0f - disabledValue * 0.35f));

    const auto icon = iconColor.get();
    const glm::vec4 iconFinal(glm::mix(glm::vec3(icon), glm::vec3(0.55f), disabledValue * 0.75f),
                              icon.a * (1.0f - disabledValue * 0.5f));

    backgroundNode->style("color", (int)f4ToRgba(background));
    iconNode->style("color", (int)f4ToRgba(iconFinal))
        ->style("svgPath", stylesStorage.get<std::string>("svgPath", ""))
        ->style("svgSize", stylesStorage.get<glm::vec2>("svgSize", glm::vec2(14.0f, 14.0f)))
        ->style("factor", stylesStorage.get<float>("factor", 0.08f));

    OMDemiurgeContainerNode::update();
}

auto OMDemiurgeIconButton::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    captureMouse();
    pressAmount.animateTo(1.0f, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
    return Handled;
}

auto OMDemiurgeIconButton::processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    pressAmount.animateTo(0.0f, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));

    if (containsPoint(x, y))
    {
        handle();
    }

    return Handled;
}

auto OMDemiurgeIconButton::processMouseEnter(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    hoverAmount.animateTo(1.0f, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
    return Handled;
}

auto OMDemiurgeIconButton::processMouseExit(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    hoverAmount.animateTo(0.0f, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
    return Handled;
}

void OMDemiurgeIconButton::setIcon(const std::string &svgPath, float viewBox)
{
    stylesStorage.put("svgPath", svgPath);
    stylesStorage.put("svgSize", glm::vec2(viewBox, viewBox));
}

void OMDemiurgeIconButton::setIconColor(glm::vec4 color)
{
    iconColor.animateTo(color, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
}

void OMDemiurgeIconButton::setBackgroundColor(glm::vec4 color)
{
    backgroundColor.animateTo(color, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
}

void OMDemiurgeIconButton::setOnClick(std::function<void()> handler)
{
    handle = std::move(handler);
}

void OMDemiurgeIconButton::setAnimationSpeed(float seconds)
{
    stylesStorage.put("animation_speed", seconds);
}

void OMDemiurgeIconButton::setDisabled(bool value, bool animate)
{
    if (disabled == value)
    {
        return;
    }

    disabled = value;

    if (animate)
    {
        disabledAmount.animateTo(disabled ? 1.0f : 0.0f, easeOutCirc<float>,
                                 stylesStorage.get<float>("animation_speed", 0.18f));
    }
    else
    {
        disabledAmount.setTo(disabled ? 1.0f : 0.0f);
    }
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
