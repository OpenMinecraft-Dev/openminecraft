#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_switch.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

#include <algorithm>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
static constexpr float TrackWidth = 34.0f;
static constexpr float TrackHeight = 18.0f;
static constexpr float ThumbSize = 14.0f;

OMDemiurgeSwitch::OMDemiurgeSwitch()
    : onAmount(0.0f), hoverAmount(0.0f), pressAmount(0.0f), disabledAmount(0.0f),
      trackOff(glm::vec4(0.22f, 0.22f, 0.26f, 0.9f)), trackOn(glm::vec4(0.0f, 0.55f, 0.75f, 1.0f)),
      thumbColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
{
    stylesStorage.put("width", OMDemiurgeSize::fit());
    stylesStorage.put("height", OMDemiurgeSize::fit());

    trackNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"width", OMDemiurgeSize::pixels(TrackWidth)},
        {"height", OMDemiurgeSize::pixels(TrackHeight)},
        {"radius", glm::vec4(TrackHeight * 0.5f)},
    });

    thumbNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"position", OMDemiurgePosition::Absolute},
        {"offsetX", OMDemiurgeSize::pixels(2.0f)},
        {"offsetY", OMDemiurgeSize::pixels((TrackHeight - ThumbSize) * 0.5f)},
        {"width", OMDemiurgeSize::pixels(ThumbSize)},
        {"height", OMDemiurgeSize::pixels(ThumbSize)},
        {"radius", glm::vec4(ThumbSize * 0.5f)},
    });

    trackNode->mount(thumbNode);
    this->mountDirect(trackNode);
}

OMDemiurgeSwitch::~OMDemiurgeSwitch() = default;

auto OMDemiurgeSwitch::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    OMDemiurgeContainerNode::submit(handler, depth);
}

void OMDemiurgeSwitch::update()
{
    const float amount = std::clamp(onAmount.get(), 0.0f, 1.0f);
    const float hover = std::clamp(hoverAmount.get(), 0.0f, 1.0f);
    const float press = std::clamp(pressAmount.get(), 0.0f, 1.0f);
    const float disabledValue = std::clamp(disabledAmount.get(), 0.0f, 1.0f);

    glm::vec4 track = trackOff.get() * (1.0f - amount) + trackOn.get() * amount;
    track = glm::vec4(glm::mix(glm::vec3(track), glm::vec3(1.0f), hover * 0.15f), track.a * (1.0f - press * 0.12f));
    track = glm::vec4(glm::mix(glm::vec3(track), glm::vec3(0.30f), disabledValue * 0.75f),
                      track.a * (1.0f - disabledValue * 0.35f));

    const auto thumb = thumbColor.get();
    const glm::vec4 thumbFinal(glm::mix(glm::vec3(thumb), glm::vec3(0.55f), disabledValue * 0.75f), thumb.a);

    trackNode->style("color", (int)f4ToRgba(track))
        ->style("radius", glm::vec4(TrackHeight * 0.5f));

    const float travel = TrackWidth - ThumbSize - 4.0f;
    thumbNode->style("offsetX", OMDemiurgeSize::pixels(2.0f + travel * amount))
        ->style("color", (int)f4ToRgba(thumbFinal))
        ->style("radius", glm::vec4(ThumbSize * 0.5f));

    OMDemiurgeContainerNode::update();
}

auto OMDemiurgeSwitch::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    pressAmount.animateTo(1.0f, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
    return Handled;
}

auto OMDemiurgeSwitch::processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    pressAmount.animateTo(0.0f, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
    setChecked(!checked);
    return Handled;
}

auto OMDemiurgeSwitch::processMouseEnter(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    hoverAmount.animateTo(1.0f, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
    return Handled;
}

auto OMDemiurgeSwitch::processMouseExit(float x, float y) -> OMDemiurgeEventResult
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

void OMDemiurgeSwitch::setChecked(bool value, bool animate)
{
    if (checked == value)
    {
        return;
    }

    checked = value;

    if (animate)
    {
        onAmount.animateTo(checked ? 1.0f : 0.0f, easeOutCubic<float>,
                           stylesStorage.get<float>("animation_speed", 0.18f));
    }
    else
    {
        onAmount.setTo(checked ? 1.0f : 0.0f);
    }

    handle(checked);
}

void OMDemiurgeSwitch::setOnChange(std::function<void(bool)> handler)
{
    handle = std::move(handler);
}

void OMDemiurgeSwitch::setTrackColors(glm::vec4 offColor, glm::vec4 onColor)
{
    const float speed = stylesStorage.get<float>("animation_speed", 0.18f);
    trackOff.animateTo(offColor, easeOutCirc<float>, speed);
    trackOn.animateTo(onColor, easeOutCirc<float>, speed);
}

void OMDemiurgeSwitch::setThumbColor(glm::vec4 color)
{
    thumbColor.animateTo(color, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.18f));
}

void OMDemiurgeSwitch::setAnimationSpeed(float seconds)
{
    stylesStorage.put("animation_speed", seconds);
}

void OMDemiurgeSwitch::setDisabled(bool value, bool animate)
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
