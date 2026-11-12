#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_slider.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

#include <algorithm>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
static constexpr float TrackHeight = 4.0f;
static constexpr float ThumbSize = 12.0f;

OMDemiurgeSlider::OMDemiurgeSlider()
    : hoverAmount(0.0f), pressAmount(0.0f), disabledAmount(0.0f),
      trackColor(glm::vec4(0.22f, 0.22f, 0.26f, 0.9f)), fillColor(glm::vec4(0.0f, 0.55f, 0.75f, 1.0f)),
      thumbColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
{
    setFocusable(true);

    stylesStorage.put("width", OMDemiurgeSize::pixels(160.0f));
    stylesStorage.put("height", OMDemiurgeSize::pixels(ThumbSize));

    trackNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"position", OMDemiurgePosition::Absolute},
        {"offsetX", OMDemiurgeSize::pixels(0.0f)},
        {"offsetY", OMDemiurgeSize::pixels((ThumbSize - TrackHeight) * 0.5f)},
        {"width", OMDemiurgeSize::fill()},
        {"height", OMDemiurgeSize::pixels(TrackHeight)},
        {"radius", glm::vec4(TrackHeight * 0.5f)},
    });

    fillNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"position", OMDemiurgePosition::Absolute},
        {"offsetX", OMDemiurgeSize::pixels(0.0f)},
        {"offsetY", OMDemiurgeSize::pixels((ThumbSize - TrackHeight) * 0.5f)},
        {"width", OMDemiurgeSize::pixels(0.0f)},
        {"height", OMDemiurgeSize::pixels(TrackHeight)},
        {"radius", glm::vec4(TrackHeight * 0.5f)},
    });

    thumbNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"position", OMDemiurgePosition::Absolute},
        {"offsetX", OMDemiurgeSize::pixels(0.0f)},
        {"offsetY", OMDemiurgeSize::pixels(0.0f)},
        {"width", OMDemiurgeSize::pixels(ThumbSize)},
        {"height", OMDemiurgeSize::pixels(ThumbSize)},
        {"radius", glm::vec4(ThumbSize * 0.5f)},
    });

    this->mountDirect(trackNode);
    this->mountDirect(fillNode);
    this->mountDirect(thumbNode);
}

OMDemiurgeSlider::~OMDemiurgeSlider() = default;

auto OMDemiurgeSlider::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    OMDemiurgePane::submit(handler, depth);
}

void OMDemiurgeSlider::update()
{
    const float hover = std::clamp(hoverAmount.get(), 0.0f, 1.0f);
    const float press = std::clamp(pressAmount.get() + (dragging ? 1.0f : 0.0f), 0.0f, 1.0f);
    const float disabledValue = std::clamp(disabledAmount.get(), 0.0f, 1.0f);

    auto finish = [&](glm::vec4 color) -> glm::vec4 {
        glm::vec3 rgb = glm::mix(glm::vec3(color), glm::vec3(1.0f), hover * 0.10f);
        rgb = glm::mix(rgb, glm::vec3(0.0f), press * 0.10f);
        rgb = glm::mix(rgb, glm::vec3(0.30f), disabledValue * 0.75f);
        return glm::vec4(rgb, color.a * (1.0f - disabledValue * 0.35f));
    };

    float width = 160.0f;

    try
    {
        auto bound = fetchLayoutBound();

        if (bound.width > 0.0f)
        {
            width = bound.width;
        }
    }
    catch (std::bad_any_cast &)
    {
    }

    const float travel = std::max(0.0f, width - ThumbSize);
    const float fillWidth = travel * value + TrackHeight * 0.5f;

    trackNode->style("color", (int)f4ToRgba(finish(trackColor.get())));
    fillNode->style("color", (int)f4ToRgba(finish(fillColor.get())))
        ->style("width", OMDemiurgeSize::pixels(std::clamp(fillWidth, 0.0f, width)));
    thumbNode->style("color", (int)f4ToRgba(finish(thumbColor.get())))
        ->style("offsetX", OMDemiurgeSize::pixels(travel * value));

    OMDemiurgePane::update();
}

auto OMDemiurgeSlider::updateFromMouse(float x) -> bool
{
    try
    {
        auto bound = fetchLayoutBound();

        if (bound.width <= 0.0f)
        {
            return false;
        }

        const float normalized = std::clamp((x - bound.x) / bound.width, 0.0f, 1.0f);

        if (std::abs(normalized - value) > 0.0001f)
        {
            value = normalized;
            handle(value);
        }

        return true;
    }
    catch (std::bad_any_cast &)
    {
        return false;
    }
}

auto OMDemiurgeSlider::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    captureMouse();
    dragging = true;
    updateFromMouse(x);
    return Handled;
}

auto OMDemiurgeSlider::processMouseMove(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled || !dragging)
    {
        return Ignored;
    }

    updateFromMouse(x);
    return Handled;
}

auto OMDemiurgeSlider::processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    dragging = false;
    updateFromMouse(x);
    releaseMouse();
    return Handled;
}

auto OMDemiurgeSlider::processMouseEnter(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    hoverAmount.animateTo(1.0f, easeOutCirc<float>, stylesStorage.get<float>("animationSpeed", 0.18f));
    return Handled;
}

auto OMDemiurgeSlider::processMouseExit(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    const float speed = stylesStorage.get<float>("animationSpeed", 0.18f);
    hoverAmount.animateTo(0.0f, easeOutCirc<float>, speed);

    // Mouse-up outside the control is not routed back to us, so a drag that leaves
    // the bounds must be cancelled here; otherwise the thumb keeps following the
    // pointer when it re-enters without any button pressed.
    if (dragging)
    {
        dragging = false;
        pressAmount.animateTo(0.0f, easeOutCirc<float>, speed);
    }

    return Handled;
}

void OMDemiurgeSlider::setValue(float v, bool notify)
{
    const float next = std::clamp(v, 0.0f, 1.0f);

    if (std::abs(next - value) <= 0.0001f)
    {
        return;
    }

    value = next;

    if (notify)
    {
        handle(value);
    }
}

void OMDemiurgeSlider::setOnChange(std::function<void(float)> handler)
{
    handle = std::move(handler);
}

void OMDemiurgeSlider::setColors(glm::vec4 track, glm::vec4 fill, glm::vec4 thumb)
{
    const float speed = stylesStorage.get<float>("animationSpeed", 0.18f);
    trackColor.animateTo(track, easeOutCirc<float>, speed);
    fillColor.animateTo(fill, easeOutCirc<float>, speed);
    thumbColor.animateTo(thumb, easeOutCirc<float>, speed);
}

void OMDemiurgeSlider::setAnimationSpeed(float seconds)
{
    stylesStorage.put("animationSpeed", seconds);
}

void OMDemiurgeSlider::setDisabled(bool value, bool animate)
{
    if (disabled == value)
    {
        return;
    }

    disabled = value;

    if (!disabled)
    {
        dragging = false;
    }

    if (animate)
    {
        disabledAmount.animateTo(disabled ? 1.0f : 0.0f, easeOutCirc<float>,
                                 stylesStorage.get<float>("animationSpeed", 0.18f));
    }
    else
    {
        disabledAmount.setTo(disabled ? 1.0f : 0.0f);
    }
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
