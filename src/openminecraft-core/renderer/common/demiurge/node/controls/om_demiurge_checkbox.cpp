#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_checkbox.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_svg.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

#include <algorithm>
#include <array>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Filled check mark in a 14x14 view box (strokes are not supported by the SVG
// rasterizer, so the tick is a closed polygon).
static constexpr const char *CheckPath =
    "M6.956 2.905a.7.7 0 0 1 .989.99l-3.2 3.2a.7.7 0 0 1-.99 0l-1.7-1.7a.7.7 0 0 1 "
    ".99-.99l.265.264c.419.419.628.628.886.649a1 1 0 0 0 .11 0c.257-.02.466-.23.885-.649z";

OMDemiurgeCheckBox::OMDemiurgeCheckBox(geom::OMFontSet *fontset)
    : checkAmount(0.0f), boxUnchecked(glm::vec4(0.1f, 0.1f, 0.12f, 0.65f)),
      boxChecked(glm::vec4(0.0f, 0.55f, 0.75f, 1.0f)), checkColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)),
      labelColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)), hoverAmount(0.0f), pressAmount(0.0f), disabledAmount(0.0f)
{
    setFocusable(true);

    stylesStorage.put("flexDirection", OMDemiurgeDirection::Row);
    stylesStorage.put("alignItems", OMDemiurgeAlign::Center);
    stylesStorage.put("flexGap", OMDemiurgeSize::pixels(6.0f));
    stylesStorage.put("width", OMDemiurgeSize::fit());
    stylesStorage.put("height", OMDemiurgeSize::fit());

    boxNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"width", OMDemiurgeSize::pixels(16.0f)},
        {"height", OMDemiurgeSize::pixels(16.0f)},
        {"radius", glm::vec4(4.0f)},
    });

    checkNode = std::make_shared<OMDemiurgeSvgNode>()->style({
        {"position", OMDemiurgePosition::Absolute},
        {"width", OMDemiurgeSize::pixels(14.0f)},
        {"height", OMDemiurgeSize::pixels(14.0f)},
        {"svgPath", std::string(CheckPath)},
        {"svgSize", glm::vec2(8.0f, 8.0f)},
        {"factor", 0.08f},
    });

    boxNode->mount(checkNode);
    this->mountDirect(boxNode);

    if (fontset != nullptr)
    {
        textNode = std::make_shared<OMDemiurgeTextSdfNode>(fontset);
        this->mountDirect(textNode);
    }
}

OMDemiurgeCheckBox::~OMDemiurgeCheckBox() = default;

auto OMDemiurgeCheckBox::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    OMDemiurgeContainerNode::submit(handler, depth);
}

void OMDemiurgeCheckBox::update()
{
    const float amount = std::clamp(checkAmount.get(), 0.0f, 1.0f);
    const auto unchecked = boxUnchecked.get();
    const auto checkedBox = boxChecked.get();
    const auto tick = checkColor.get();

    glm::vec4 boxColor = unchecked * (1.0f - amount) + checkedBox * amount;

    // Hover brightens, press darkens (same interaction idea as OMDemiurgeButton).
    const float hover = std::clamp(hoverAmount.get(), 0.0f, 1.0f);
    const float press = std::clamp(pressAmount.get(), 0.0f, 1.0f);
    const float hoverBoost = stylesStorage.get<float>("hover_boost", 0.18f);
    const float pressBoost = stylesStorage.get<float>("press_boost", 0.12f);

    glm::vec3 rgb = glm::mix(glm::vec3(boxColor), glm::vec3(1.0f), hover * hoverBoost);
    rgb = glm::mix(rgb, glm::vec3(0.0f), press * pressBoost);
    boxColor = glm::vec4(rgb, boxColor.a);

    // Disabled: desaturate towards a muted grey and fade a little.
    const float disabledValue = std::clamp(disabledAmount.get(), 0.0f, 1.0f);
    boxColor = glm::vec4(glm::mix(glm::vec3(boxColor), glm::vec3(0.30f), disabledValue * 0.75f),
                         boxColor.a * (1.0f - disabledValue * 0.35f));
    boxNode->style("color", (int)f4ToRgba(boxColor))
        ->style("radius", stylesStorage.get<glm::vec4>("radius", glm::vec4(4.0f)));

    const glm::vec3 tickRgb = glm::mix(glm::vec3(tick), glm::vec3(0.55f), disabledValue * 0.75f);
    checkNode->style("color", (int)f4ToRgba(glm::vec4(tickRgb, tick.a * amount * (1.0f - disabledValue * 0.85f))))
        ->style("svgPath", stylesStorage.get<std::string>("svgPath", std::string(CheckPath)))
        ->style("svgSize", glm::vec2(14.0f, 14.0f));

    if (textNode != nullptr)
    {
        const int textHeight = stylesStorage.get<int>("textheight", 14);

        // Text glyphs are laid out from the top of their node (font ascent included), so
        // center the visible ink against the box instead of centering the layout box.
        const float textOffset = stylesStorage.get<float>("text_offset", -0.21f * static_cast<float>(textHeight));

        textNode->style("text", stylesStorage.get<std::string>("label", "Checkbox"));
        textNode->style("textheight", textHeight);
        textNode->style("offsetY", OMDemiurgeSize::pixels(textOffset));
        const auto label = labelColor.get();
        const glm::vec3 labelRgb = glm::mix(glm::vec3(label), glm::vec3(0.55f), disabledValue * 0.75f);
        textNode->style("color", (int)f4ToRgba(glm::vec4(labelRgb, label.a)));
    }

    OMDemiurgeContainerNode::update();
}

auto OMDemiurgeCheckBox::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    captureMouse();
    pressAmount.animateTo(1.0f, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
    return Handled;
}

auto OMDemiurgeCheckBox::processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    pressAmount.animateTo(0.0f, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));

    if (containsPoint(x, y))
    {
        toggle();
    }

    return Handled;
}

auto OMDemiurgeCheckBox::processMouseEnter(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    hoverAmount.animateTo(1.0f, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
    return Handled;
}

auto OMDemiurgeCheckBox::processMouseExit(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    const float speed = stylesStorage.get<float>("animation_speed", 0.18f);
    hoverAmount.animateTo(0.0f, easeOutCirc<float>, speed);
    pressAmount.animateTo(0.0f, easeOutCirc<float>, speed);
    return Handled;
}

void OMDemiurgeCheckBox::setChecked(bool value, bool animate)
{
    if (checked == value)
    {
        return;
    }

    checked = value;

    if (animate)
    {
        checkAmount.animateTo(checked ? 1.0f : 0.0f, easeOutCubic<float>,
                              stylesStorage.get<float>("animation_speed", 0.18f));
    }
    else
    {
        checkAmount.setTo(checked ? 1.0f : 0.0f);
    }

    handle(checked);
}

void OMDemiurgeCheckBox::toggle()
{
    setChecked(!checked);
}

void OMDemiurgeCheckBox::setOnChange(std::function<void(bool)> handler)
{
    handle = std::move(handler);
}

void OMDemiurgeCheckBox::setCheckColor(glm::vec4 color)
{
    checkColor.animateTo(color, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
}

void OMDemiurgeCheckBox::setBoxColors(glm::vec4 uncheckedColor, glm::vec4 checkedColor)
{
    const float speed = stylesStorage.get<float>("animation_speed", 0.18f);
    boxUnchecked.animateTo(uncheckedColor, easeOutCirc<float>, speed);
    boxChecked.animateTo(checkedColor, easeOutCirc<float>, speed);
}

void OMDemiurgeCheckBox::setTextColor(glm::vec4 color)
{
    labelColor.animateTo(color, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
}

void OMDemiurgeCheckBox::setLabel(const std::string &label)
{
    stylesStorage.put("label", label);
}

void OMDemiurgeCheckBox::setDisabled(bool value, bool animate)
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

void OMDemiurgeCheckBox::setAnimationSpeed(float seconds)
{
    stylesStorage.put("animation_speed", seconds);
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
