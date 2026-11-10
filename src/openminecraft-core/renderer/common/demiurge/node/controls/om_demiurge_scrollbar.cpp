#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_scrollbar.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "glm/ext/vector_float4.hpp"

#include <algorithm>
#include <cmath>

namespace openminecraft::renderer::common::demiurge::node::controls
{
OMDemiurgeScrollBar::OMDemiurgeScrollBar() : disabledAmount(0.0f)
{
    stylesStorage.put("width", OMDemiurgeSize::pixels(4.0f));
    stylesStorage.put("height", OMDemiurgeSize::pixels(160.0f));
    stylesStorage.put("radius", glm::vec4(3.0f));
    stylesStorage.put("color", (int)0x00d4ffff);
    stylesStorage.put("track_color", (int)0);
    stylesStorage.put("min_thumb", 24.0f);
    stylesStorage.put("scroll_step", 40.0f);
    stylesStorage.put("overscroll_curve", 2.0f);
    stylesStorage.put("overscroll_range", 1.0f);
    stylesStorage.put("animation_speed", 0.18f);

    trackNode = std::make_shared<OMDemiurgeRectNode>();
    trackNode->style({{"position", OMDemiurgePosition::Absolute},
                      {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                      {"offsetY", OMDemiurgeSize::pixels(0.0f)},
                      {"width", OMDemiurgeSize::fill()},
                      {"height", OMDemiurgeSize::fill()},
                      {"color", (int)0},
                      {"radius", glm::vec4(3.0f)}});
    this->mountDirect(trackNode);

    thumbNode = std::make_shared<OMDemiurgeRectNode>();
    thumbNode->style({{"position", OMDemiurgePosition::Absolute},
                      {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                      {"offsetY", OMDemiurgeSize::pixels(0.0f)},
                      {"width", OMDemiurgeSize::fill()},
                      {"height", OMDemiurgeSize::pixels(24.0f)},
                      {"color", (int)0x00d4ffff},
                      {"radius", glm::vec4(3.0f)}});
    this->mountDirect(thumbNode);
}

OMDemiurgeScrollBar::~OMDemiurgeScrollBar() = default;

void OMDemiurgeScrollBar::setOrientation(OMDemiurgeScrollBarOrientation value)
{
    if (orientation == value)
    {
        return;
    }

    orientation = value;
    dragging = false;
}

auto OMDemiurgeScrollBar::getMaxScrollOffset() const -> float
{
    return std::max(0.0f, contentSize - viewSize);
}

auto OMDemiurgeScrollBar::thumbTravel() const -> float
{
    return std::max(0.0f, trackLength - thumbLength);
}

auto OMDemiurgeScrollBar::getOverscroll() const -> float
{
    const float maxScroll = getMaxScrollOffset();

    if (scrollOffset < 0.0f)
    {
        return -scrollOffset;
    }

    return std::max(0.0f, scrollOffset - maxScroll);
}

auto OMDemiurgeScrollBar::axisCoordinate(float x, float y) const -> float
{
    return orientation == OMDemiurgeScrollBarOrientation::Vertical ? y : x;
}

void OMDemiurgeScrollBar::setMetrics(float view, float content)
{
    viewSize = std::max(0.0f, view);
    contentSize = std::max(0.0f, content);
}

void OMDemiurgeScrollBar::setViewSize(float value)
{
    setMetrics(value, contentSize);
}

void OMDemiurgeScrollBar::setContentSize(float value)
{
    setMetrics(viewSize, value);
}

void OMDemiurgeScrollBar::setScrollOffset(float value, bool notify)
{
    if (std::abs(value - scrollOffset) < 1e-4f)
    {
        return;
    }

    scrollOffset = value;

    if (notify && handleScroll)
    {
        handleScroll(scrollOffset);
    }
}

void OMDemiurgeScrollBar::setOnScroll(std::function<void(float)> handler)
{
    handleScroll = std::move(handler);
}

void OMDemiurgeScrollBar::update()
{
    const auto radius = stylesStorage.get<glm::vec4>("radius", glm::vec4(3.0f));
    const float minThumb = std::max(1.0f, stylesStorage.get<float>("min_thumb", 24.0f));
    const bool vertical = orientation == OMDemiurgeScrollBarOrientation::Vertical;

    float width = 6.0f;
    float height = 160.0f;

    try
    {
        auto bound = fetchLayoutBound();
        width = std::max(1.0f, bound.width);
        height = std::max(1.0f, bound.height);
    }
    catch (std::bad_any_cast &)
    {
    }

    trackLength = vertical ? height : width;

    const float maxScroll = getMaxScrollOffset();
    const float ratio = contentSize > 0.0f ? std::clamp(viewSize / contentSize, 0.0f, 1.0f) : 0.0f;
    const bool scrollable = maxScroll > 0.0f && viewSize > 0.0f;

    thumbLength = scrollable ? std::clamp(ratio * trackLength, minThumb, trackLength) : trackLength;

    // Out-of-range offsets (overscroll) compress the thumb against the end it
    // overshoots. The compression is non-linear: with one full view height of
    // overscroll the thumb collapses into a circle (length == thickness).
    const float overscroll = getOverscroll();
    const float thickness = vertical ? width : height;
    const float curve = std::max(0.1f, stylesStorage.get<float>("overscroll_curve", 2.0f));
    const float range = std::max(1.0f, viewSize * std::max(0.01f, stylesStorage.get<float>("overscroll_range", 1.0f)));
    const float progress = std::clamp(overscroll / range, 0.0f, 1.0f);
    const float eased = std::pow(progress, curve);
    const float collapsed = thumbLength - (thumbLength - thickness) * eased;

    visualThumbLength = scrollable ? std::max(thickness, collapsed) : thumbLength;

    const float travel = thumbTravel();
    float thumbPos = 0.0f;

    if (scrollable && maxScroll > 0.0f && travel > 0.0f)
    {
        if (scrollOffset <= 0.0f)
        {
            thumbPos = 0.0f;
        }
        else if (scrollOffset >= maxScroll)
        {
            thumbPos = std::max(0.0f, trackLength - visualThumbLength);
        }
        else
        {
            thumbPos = (scrollOffset / maxScroll) * travel;
        }
    }

    const float disabledValue = std::clamp(disabledAmount.get(), 0.0f, 1.0f);
    auto applyDisabled = [&](int color) -> int {
        if (disabledValue <= 0.0f)
        {
            return color;
        }

        const uint32_t rgba = static_cast<uint32_t>(color);
        const uint32_t alpha = static_cast<uint32_t>(static_cast<float>(rgba & 0xff) * (1.0f - disabledValue * 0.6f));
        return static_cast<int>((rgba & 0xffffff00u) | alpha);
    };

    const float cornerRadius = std::min(std::max(radius.x, 0.0f), thickness * 0.5f);
    const glm::vec4 thumbRadius(cornerRadius);

    trackNode->style("color", (int)stylesStorage.get<int>("track_color", (int)0))
        ->style("radius", radius);
    thumbNode->style("radius", thumbRadius)
        ->style("color", (int)(scrollable ? applyDisabled(stylesStorage.get<int>("color", (int)0x00d4ffff)) : 0));

    if (vertical)
    {
        thumbNode->style("width", OMDemiurgeSize::fill())
            ->style("height", OMDemiurgeSize::pixels(visualThumbLength))
            ->style("offsetX", OMDemiurgeSize::pixels(0.0f))
            ->style("offsetY", OMDemiurgeSize::pixels(thumbPos));
    }
    else
    {
        thumbNode->style("width", OMDemiurgeSize::pixels(visualThumbLength))
            ->style("height", OMDemiurgeSize::fill())
            ->style("offsetX", OMDemiurgeSize::pixels(thumbPos))
            ->style("offsetY", OMDemiurgeSize::pixels(0.0f));
    }

    OMDemiurgeContainerNode::update();
}

auto OMDemiurgeScrollBar::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled || button != 1 || !containsPoint(x, y))
    {
        return Ignored;
    }

    if (thumbNode->containsPoint(x, y))
    {
        dragging = true;
        dragStartPointer = axisCoordinate(x, y);
        dragStartOffset = scrollOffset;
        captureMouse();
        return Handled;
    }

    // Track click: center the thumb at the clicked position.
    const float travel = thumbTravel();

    if (travel > 0.0f && getMaxScrollOffset() > 0.0f)
    {
        try
        {
            auto bound = fetchLayoutBound();
            const float local = orientation == OMDemiurgeScrollBarOrientation::Vertical ? y - bound.y : x - bound.x;
            const float target = std::clamp(local - visualThumbLength * 0.5f, 0.0f, travel);
            setScrollOffset(target / travel * getMaxScrollOffset(), true);
            return Handled;
        }
        catch (std::bad_any_cast &)
        {
        }
    }

    return Ignored;
}

auto OMDemiurgeScrollBar::processMouseMove(float x, float y) -> OMDemiurgeEventResult
{
    if (!dragging)
    {
        return Ignored;
    }

    const float travel = thumbTravel();

    if (travel <= 0.0f)
    {
        return Handled;
    }

    const float delta = axisCoordinate(x, y) - dragStartPointer;
    setScrollOffset(dragStartOffset + delta / travel * getMaxScrollOffset(), true);
    return Handled;
}

auto OMDemiurgeScrollBar::processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (!dragging || button != 1)
    {
        return Ignored;
    }

    dragging = false;
    releaseMouse();
    return Handled;
}

auto OMDemiurgeScrollBar::processMouseScroll(float x, float y, float wx, float wy) -> OMDemiurgeEventResult
{
    if (disabled || getMaxScrollOffset() <= 0.0f || !containsPoint(x, y))
    {
        return Ignored;
    }

    const float step = std::max(1.0f, stylesStorage.get<float>("scroll_step", 40.0f));
    setScrollOffset(scrollOffset - wy * step, true);
    return Handled;
}

void OMDemiurgeScrollBar::setDisabled(bool value, bool animate)
{
    if (disabled == value)
    {
        return;
    }

    disabled = value;

    if (animate)
    {
        disabledAmount.animateTo(value ? 1.0f : 0.0f, animation::easeOutCirc<float>,
                                 stylesStorage.get<float>("animation_speed", 0.18f));
    }
    else
    {
        disabledAmount.setTo(value ? 1.0f : 0.0f);
    }
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
