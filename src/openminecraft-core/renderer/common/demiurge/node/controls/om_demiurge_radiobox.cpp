#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_radiobox.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

#include <algorithm>
#include <array>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
static constexpr float RadioSize = 16.0f;
static constexpr float DotSize = 7.0f;

OMDemiurgeRadioGroup::OMDemiurgeRadioGroup()
{
    stylesStorage.put("flexDirection", OMDemiurgeDirection::Column);
    stylesStorage.put("alignItems", OMDemiurgeAlign::FlexStart);
    stylesStorage.put("flexGap", OMDemiurgeSize::pixels(6.0f));
    stylesStorage.put("width", OMDemiurgeSize::fit());
    stylesStorage.put("height", OMDemiurgeSize::fit());
}

void OMDemiurgeRadioGroup::add(const std::shared_ptr<OMDemiurgeRadioBox> &box)
{
    if (!box)
    {
        return;
    }

    for (auto &m : members)
    {
        if (auto current = m.lock(); current == box)
        {
            return;
        }
    }

    members.push_back(box);
    this->mount(box);
}

void OMDemiurgeRadioGroup::select(const std::shared_ptr<OMDemiurgeRadioBox> &box)
{
    for (auto it = members.begin(); it != members.end();)
    {
        auto current = it->lock();

        if (!current)
        {
            it = members.erase(it);
            continue;
        }

        if (current != box && !multiSelect)
        {
            current->applySelection(false, true, false);
        }

        ++it;
    }

    if (box)
    {
        box->applySelection(true, true, true);
    }
}

void OMDemiurgeRadioGroup::click(const std::shared_ptr<OMDemiurgeRadioBox> &box)
{
    if (!box)
    {
        return;
    }

    if (multiSelect)
    {
        box->applySelection(!box->isSelected(), true, true);
    }
    else
    {
        select(box);
    }
}

void OMDemiurgeRadioGroup::setDirection(OMDemiurgeDirection direction)
{
    stylesStorage.put("flexDirection", direction);
    stylesStorage.put("alignItems", direction == OMDemiurgeDirection::Row ? OMDemiurgeAlign::Center
                                                                          : OMDemiurgeAlign::FlexStart);
}

void OMDemiurgeRadioGroup::setGap(float pixels)
{
    stylesStorage.put("flexGap", OMDemiurgeSize::pixels(pixels));
}

void OMDemiurgeRadioGroup::setMultiSelect(bool value)
{
    multiSelect = value;
}

OMDemiurgeRadioBox::OMDemiurgeRadioBox(geom::OMFontSet *fontset)
    : selectAmount(0.0f), boxUnselected(glm::vec4(0.1f, 0.1f, 0.12f, 0.65f)),
      boxSelected(glm::vec4(0.0f, 0.55f, 0.75f, 1.0f)), dotColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)),
      labelColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)), hoverAmount(0.0f), pressAmount(0.0f), disabledAmount(0.0f)
{
    setFocusable(true);

    stylesStorage.put("flexDirection", OMDemiurgeDirection::Row);
    stylesStorage.put("alignItems", OMDemiurgeAlign::Center);
    stylesStorage.put("flexGap", OMDemiurgeSize::pixels(6.0f));
    stylesStorage.put("width", OMDemiurgeSize::fit());
    stylesStorage.put("height", OMDemiurgeSize::fit());

    boxNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"width", OMDemiurgeSize::pixels(RadioSize)},
        {"height", OMDemiurgeSize::pixels(RadioSize)},
        {"radius", glm::vec4(RadioSize * 0.5f)},
    });

    dotNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"position", OMDemiurgePosition::Absolute},
        {"offsetX", OMDemiurgeSize::pixels((RadioSize - DotSize) * 0.5f)},
        {"offsetY", OMDemiurgeSize::pixels((RadioSize - DotSize) * 0.5f)},
        {"width", OMDemiurgeSize::pixels(DotSize)},
        {"height", OMDemiurgeSize::pixels(DotSize)},
        {"radius", glm::vec4(DotSize * 0.5f)},
    });

    boxNode->mount(dotNode);
    this->mountDirect(boxNode);

    if (fontset != nullptr)
    {
        textNode = std::make_shared<OMDemiurgeTextSdfNode>(fontset);
        this->mountDirect(textNode);
    }
}

OMDemiurgeRadioBox::~OMDemiurgeRadioBox() = default;

auto OMDemiurgeRadioBox::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    OMDemiurgeContainerNode::submit(handler, depth);
}

void OMDemiurgeRadioBox::update()
{
    const float amount = std::clamp(selectAmount.get(), 0.0f, 1.0f);
    const auto unselected = boxUnselected.get();
    const auto selectedBox = boxSelected.get();
    const auto dot = dotColor.get();

    glm::vec4 boxColor = unselected * (1.0f - amount) + selectedBox * amount;

    const float hover = std::clamp(hoverAmount.get(), 0.0f, 1.0f);
    const float press = std::clamp(pressAmount.get(), 0.0f, 1.0f);
    const float hoverBoost = stylesStorage.get<float>("hover_boost", 0.18f);
    const float pressBoost = stylesStorage.get<float>("press_boost", 0.12f);

    glm::vec3 rgb = glm::mix(glm::vec3(boxColor), glm::vec3(1.0f), hover * hoverBoost);
    rgb = glm::mix(rgb, glm::vec3(0.0f), press * pressBoost);
    boxColor = glm::vec4(rgb, boxColor.a);

    const float disabledValue = std::clamp(disabledAmount.get(), 0.0f, 1.0f);
    boxColor = glm::vec4(glm::mix(glm::vec3(boxColor), glm::vec3(0.30f), disabledValue * 0.75f),
                         boxColor.a * (1.0f - disabledValue * 0.35f));

    boxNode->style("color", (int)f4ToRgba(boxColor))
        ->style("radius", glm::vec4(stylesStorage.get<float>("box_radius", RadioSize * 0.5f)));

    // Keep the dot geometry fixed and scale it in the shader: animating the layout
    // size snapped to whole pixels and made the small circle jitter.
    const float pressShrink = stylesStorage.get<float>("press_shrink", 0.35f);
    const float dotScale = amount * (1.0f - press * pressShrink);

    dotNode->style("scale", glm::vec2(dotScale))
        ->style("color", (int)f4ToRgba(glm::vec4(glm::mix(glm::vec3(dot), glm::vec3(0.55f), disabledValue * 0.75f),
                                                  dot.a * amount * (1.0f - disabledValue * 0.85f))));

    if (textNode != nullptr)
    {
        const int textHeight = stylesStorage.get<int>("textheight", 14);
        const float textOffset =
            stylesStorage.get<float>("text_offset", -0.21f * static_cast<float>(textHeight));

        textNode->style("text", stylesStorage.get<std::string>("label", "Radio"));
        textNode->style("textheight", textHeight);
        textNode->style("offsetY", OMDemiurgeSize::pixels(textOffset));
        const auto label = labelColor.get();
        const glm::vec3 labelRgb = glm::mix(glm::vec3(label), glm::vec3(0.55f), disabledValue * 0.75f);
        textNode->style("color", (int)f4ToRgba(glm::vec4(labelRgb, label.a)));
    }

    OMDemiurgeContainerNode::update();
}

auto OMDemiurgeRadioBox::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    captureMouse();
    pressAmount.animateTo(1.0f, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
    return Handled;
}

auto OMDemiurgeRadioBox::processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    pressAmount.animateTo(0.0f, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));

    if (!containsPoint(x, y))
    {
        return Handled;
    }

    if (group)
    {
        group->click(std::static_pointer_cast<OMDemiurgeRadioBox>(shared_from_this()));
    }
    else if (!selected)
    {
        applySelection(true, true, true);
    }

    return Handled;
}

auto OMDemiurgeRadioBox::processMouseEnter(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    hoverAmount.animateTo(1.0f, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
    return Handled;
}

auto OMDemiurgeRadioBox::processMouseExit(float x, float y) -> OMDemiurgeEventResult
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

void OMDemiurgeRadioBox::setSelected(bool value, bool animate)
{
    if (group && value)
    {
        group->select(std::static_pointer_cast<OMDemiurgeRadioBox>(shared_from_this()));
        return;
    }

    applySelection(value, animate, true);
}

void OMDemiurgeRadioBox::applySelection(bool value, bool animate, bool notify)
{
    if (selected == value)
    {
        return;
    }

    selected = value;

    if (animate)
    {
        selectAmount.animateTo(selected ? 1.0f : 0.0f, easeOutCubic<float>,
                               stylesStorage.get<float>("animation_speed", 0.18f));
    }
    else
    {
        selectAmount.setTo(selected ? 1.0f : 0.0f);
    }

    if (notify)
    {
        handle(selected);
    }
}

void OMDemiurgeRadioBox::setGroup(std::shared_ptr<OMDemiurgeRadioGroup> value)
{
    group = std::move(value);

    if (group)
    {
        group->add(std::static_pointer_cast<OMDemiurgeRadioBox>(shared_from_this()));
    }
}

void OMDemiurgeRadioBox::setOnChange(std::function<void(bool)> handler)
{
    handle = std::move(handler);
}

void OMDemiurgeRadioBox::setDotColor(glm::vec4 color)
{
    dotColor.animateTo(color, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
}

void OMDemiurgeRadioBox::setBoxColors(glm::vec4 unselectedColor, glm::vec4 selectedColor)
{
    const float speed = stylesStorage.get<float>("animation_speed", 0.18f);
    boxUnselected.animateTo(unselectedColor, easeOutCirc<float>, speed);
    boxSelected.animateTo(selectedColor, easeOutCirc<float>, speed);
}

void OMDemiurgeRadioBox::setTextColor(glm::vec4 color)
{
    labelColor.animateTo(color, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
}

void OMDemiurgeRadioBox::setLabel(const std::string &label)
{
    stylesStorage.put("label", label);
}

void OMDemiurgeRadioBox::setDisabled(bool value, bool animate)
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

void OMDemiurgeRadioBox::setAnimationSpeed(float seconds)
{
    stylesStorage.put("animation_speed", seconds);
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
