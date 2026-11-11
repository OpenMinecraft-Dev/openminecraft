#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_scrollpane.hpp"
#include <cstdlib>
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"

#include <algorithm>
#include <cmath>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
OMDemiurgeScrollPane::OMDemiurgeScrollPane() : disabledAmount(0.0f)
{
    stylesStorage.put("width", OMDemiurgeSize::fill());
    stylesStorage.put("height", OMDemiurgeSize::fill());
    stylesStorage.put("wheel_step", 40.0f);
    stylesStorage.put("drag_threshold", 4.0f);
    stylesStorage.put("rubber_band", 0.45f);
    stylesStorage.put("bounce_speed", 0.3f);
    stylesStorage.put("wheel_speed", 0.18f);
    stylesStorage.put("inertia_factor", 0.18f);
    stylesStorage.put("inertia_speed", 0.55f);
    stylesStorage.put("scrollbar_thickness", 4.0f);
    stylesStorage.put("animation_speed", 0.18f);

    viewportClip = std::make_shared<OMDemiurgeRectNode>();
    viewportClip->style({{"position", OMDemiurgePosition::Absolute},
                         {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                         {"offsetY", OMDemiurgeSize::pixels(0.0f)},
                         {"width", OMDemiurgeSize::fill()},
                         {"height", OMDemiurgeSize::fill()},
                         {"color", (int)0}});
    this->mountDirect(viewportClip);

    contentHost = std::make_shared<OMDemiurgeContainerNode>();
    contentHost->style({{"position", OMDemiurgePosition::Absolute},
                        {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                        {"offsetY", OMDemiurgeSize::pixels(0.0f)},
                        {"width", OMDemiurgeSize::fit()},
                        {"height", OMDemiurgeSize::fit()}});
    this->mountDirect(contentHost);

    verticalBar = std::make_shared<OMDemiurgeScrollBar>();
    verticalBar->style({{"position", OMDemiurgePosition::Absolute},
                        {"width", OMDemiurgeSize::pixels(4.0f)},
                        {"height", OMDemiurgeSize::fill()}});
    verticalBar->setAllowOverscroll(false);
    verticalBar->setVisible(false);
    verticalBar->setOnScroll([this](float value) {
        scrollY.setTo(value);
        notifyScroll();
    });
    this->mountDirect(verticalBar);

    horizontalBar = std::make_shared<OMDemiurgeScrollBar>();
    horizontalBar->setOrientation(OMDemiurgeScrollBarOrientation::Horizontal);
    horizontalBar->style({{"position", OMDemiurgePosition::Absolute},
                          {"width", OMDemiurgeSize::fill()},
                          {"height", OMDemiurgeSize::pixels(4.0f)}});
    horizontalBar->setAllowOverscroll(false);
    horizontalBar->setVisible(false);
    horizontalBar->setOnScroll([this](float value) {
        scrollX.setTo(value);
        notifyScroll();
    });
    this->mountDirect(horizontalBar);
}

OMDemiurgeScrollPane::~OMDemiurgeScrollPane() = default;

auto OMDemiurgeScrollPane::rubberBand(float raw, float maxValue) -> float
{
    const float band = std::clamp(stylesStorage.get<float>("rubber_band", 0.45f), 0.02f, 1.0f);

    if (raw < 0.0f)
    {
        return raw * band;
    }

    if (raw > maxValue)
    {
        return maxValue + (raw - maxValue) * band;
    }

    return raw;
}

void OMDemiurgeScrollPane::applyBounce()
{
    const float speed = std::max(0.01f, stylesStorage.get<float>("bounce_speed", 0.3f));
    const float targetX = std::clamp(scrollX.get(), 0.0f, maxScrollX);
    const float targetY = std::clamp(scrollY.get(), 0.0f, maxScrollY);

    if (std::abs(targetX - scrollX.get()) > 0.01f)
    {
        scrollX.animateTo(targetX, easeOutCubic<float>, speed);
    }
    else
    {
        scrollX.setTo(targetX);
    }

    if (std::abs(targetY - scrollY.get()) > 0.01f)
    {
        scrollY.animateTo(targetY, easeOutCubic<float>, speed);
    }
    else
    {
        scrollY.setTo(targetY);
    }
}

void OMDemiurgeScrollPane::applyInertia()
{
    const auto now = std::chrono::steady_clock::now();
    const float idle = std::chrono::duration<float>(now - lastDragTime).count();

    if (idle > 0.1f)
    {
        dragVelocityX = 0.0f;
        dragVelocityY = 0.0f;
    }

    const float factor = std::max(0.0f, stylesStorage.get<float>("inertia_factor", 0.18f));
    const float speed = std::max(0.05f, stylesStorage.get<float>("inertia_speed", 0.55f));
    const float minSpeed = 40.0f;

    const bool inertX = std::abs(dragVelocityX) > minSpeed && maxScrollX > 0.0f;
    const bool inertY = std::abs(dragVelocityY) > minSpeed && maxScrollY > 0.0f;

    if (inertX)
    {
        const float target = std::clamp(scrollX.get() + dragVelocityX * factor, 0.0f, maxScrollX);

        if (std::abs(target - scrollX.get()) > 0.5f)
        {
            scrollX.animateTo(target, easeOutCubic<float>, speed);
        }
    }

    if (inertY)
    {
        const float target = std::clamp(scrollY.get() + dragVelocityY * factor, 0.0f, maxScrollY);

        if (std::abs(target - scrollY.get()) > 0.5f)
        {
            scrollY.animateTo(target, easeOutCubic<float>, speed);
        }
    }
}

void OMDemiurgeScrollPane::notifyScroll()
{
    if (handleScroll)
    {
        handleScroll(scrollX.get(), scrollY.get());
    }
}

void OMDemiurgeScrollPane::setOrientation(OMDemiurgeScrollPaneOrientation value)
{
    orientation = value;
}

void OMDemiurgeScrollPane::setContentNode(const std::shared_ptr<OMDemiurgeNode> &content)
{
    if (contentNode == content)
    {
        return;
    }

    if (contentNode)
    {
        contentHost->umount(contentNode);
    }

    contentNode = content;

    if (contentNode)
    {
        contentHost->mountDirect(contentNode);
    }

    preserveScrollOnContentChange();
}

void OMDemiurgeScrollPane::preserveScrollOnContentChange()
{
    pressed = false;
    dragging = false;
    releaseMouse();

    // Preserve the current offset as much as possible. The next update compares
    // against the new content metrics and clamps it to min(offset, new max).
    lastMaxScrollX = -1.0f;
    lastMaxScrollY = -1.0f;
}

void OMDemiurgeScrollPane::setContentStretch(bool value)
{
    contentStretch = value;
}

void OMDemiurgeScrollPane::setScrollOffset(float x, float y, bool notify)
{
    scrollX.setTo(x);
    scrollY.setTo(y);

    if (notify)
    {
        notifyScroll();
    }
}

auto OMDemiurgeScrollPane::getScrollX() -> float
{
    return scrollX.get();
}

auto OMDemiurgeScrollPane::getScrollY() -> float
{
    return scrollY.get();
}

void OMDemiurgeScrollPane::setOnScroll(std::function<void(float, float)> handler)
{
    handleScroll = std::move(handler);
}

void OMDemiurgeScrollPane::update()
{
    const bool vertical = orientation != OMDemiurgeScrollPaneOrientation::Horizontal;
    const bool horizontal = orientation != OMDemiurgeScrollPaneOrientation::Vertical;

    float width = 0.0f;
    float height = 0.0f;

    try
    {
        auto bound = fetchLayoutBound();
        width = std::max(0.0f, bound.width);
        height = std::max(0.0f, bound.height);
    }
    catch (std::bad_any_cast &)
    {
    }

    const float thickness = std::max(2.0f, stylesStorage.get<float>("scrollbar_thickness", 4.0f));
    const float paneWidth = width;
    const float paneHeight = height;

    contentWidth = 0.0f;
    contentHeight = 0.0f;

    try
    {
        auto bound = contentHost->fetchLayoutBound();
        contentWidth = std::max(0.0f, bound.width);
        contentHeight = std::max(0.0f, bound.height);
    }
    catch (std::bad_any_cast &)
    {
    }

    // Reserve space for the scroll bars so they never overlap the content.
    bool verticalVisible = false;
    bool horizontalVisible = false;

    if (orientation == OMDemiurgeScrollPaneOrientation::Both)
    {
        for (int pass = 0; pass < 3; ++pass)
        {
            const float candidateW = std::max(0.0f, paneWidth - (verticalVisible ? thickness : 0.0f));
            const float candidateH = std::max(0.0f, paneHeight - (horizontalVisible ? thickness : 0.0f));
            verticalVisible = contentHeight > candidateH + 0.5f;
            horizontalVisible = contentWidth > candidateW + 0.5f;
        }
    }
    else
    {
        verticalVisible = vertical && contentHeight > paneHeight + 0.5f;
        horizontalVisible = horizontal && contentWidth > paneWidth + 0.5f;
    }

    viewWidth = std::max(0.0f, paneWidth - (verticalVisible ? thickness : 0.0f));
    viewHeight = std::max(0.0f, paneHeight - (horizontalVisible ? thickness : 0.0f));

    // Content layout: the scroll axis is free, the cross axis is constrained to
    // the viewport when stretch is enabled (the viewport excludes scroll bars).
    if (orientation == OMDemiurgeScrollPaneOrientation::Vertical)
    {
        contentHost->style("width", contentStretch ? OMDemiurgeSize::pixels(viewWidth) : OMDemiurgeSize::fit())
            ->style("height", OMDemiurgeSize::fit());
    }
    else if (orientation == OMDemiurgeScrollPaneOrientation::Horizontal)
    {
        contentHost->style("width", OMDemiurgeSize::fit())
            ->style("height", contentStretch ? OMDemiurgeSize::pixels(viewHeight) : OMDemiurgeSize::fit());
    }
    else
    {
        contentHost->style("width", OMDemiurgeSize::fit())->style("height", OMDemiurgeSize::fit());
    }

    maxScrollX = std::max(0.0f, contentWidth - viewWidth);
    maxScrollY = std::max(0.0f, contentHeight - viewHeight);

    // Only react to a meaningful shrink. The content viewport can jitter by a
    // pixel while scrollbars are laid out, and clamping on every jitter would
    // cancel wheel/bounce/inertia animations.
    const bool contentShrunk =
        maxScrollX + 1.5f < lastMaxScrollX || maxScrollY + 1.5f < lastMaxScrollY;

    lastMaxScrollX = maxScrollX;
    lastMaxScrollY = maxScrollY;

    if (contentShrunk && !dragging && !pressed)
    {
        const float currentX = scrollX.get();
        const float currentY = scrollY.get();

        if (currentX < 0.0f || currentX > maxScrollX)
        {
            scrollX.setTo(std::clamp(currentX, 0.0f, maxScrollX));
        }

        if (currentY < 0.0f || currentY > maxScrollY)
        {
            scrollY.setTo(std::clamp(currentY, 0.0f, maxScrollY));
        }
    }

    float visualX = rubberBand(scrollX.get(), maxScrollX);
    float visualY = rubberBand(scrollY.get(), maxScrollY);

    if (std::abs(visualX) < 0.05f)
    {
        visualX = 0.0f;
    }
    else if (std::abs(visualX - maxScrollX) < 0.05f)
    {
        visualX = maxScrollX;
    }

    if (std::abs(visualY) < 0.05f)
    {
        visualY = 0.0f;
    }
    else if (std::abs(visualY - maxScrollY) < 0.05f)
    {
        visualY = maxScrollY;
    }

    viewportClip->style("width", OMDemiurgeSize::pixels(viewWidth))
        ->style("height", OMDemiurgeSize::pixels(viewHeight));
    contentHost->setClipNode(viewportClip);
    contentHost->style("offsetX", OMDemiurgeSize::pixels(-visualX))
        ->style("offsetY", OMDemiurgeSize::pixels(-visualY));

    verticalBar->setVisible(verticalVisible);
    horizontalBar->setVisible(horizontalVisible);

    if (verticalVisible)
    {
        verticalBar->style("width", OMDemiurgeSize::pixels(thickness))
            ->style("height", OMDemiurgeSize::pixels(std::max(1.0f, viewHeight)))
            ->style("offsetX", OMDemiurgeSize::pixels(std::max(0.0f, paneWidth - thickness)))
            ->style("offsetY", OMDemiurgeSize::pixels(0.0f));
        verticalBar->setMetrics(viewHeight, contentHeight);
        verticalBar->setScrollOffset(visualY, false);

        const float rawY = scrollY.get();
        verticalBar->setVisualOverscroll(rawY < 0.0f ? -rawY : std::max(0.0f, rawY - maxScrollY));
    }

    if (horizontalVisible)
    {
        horizontalBar->style("width", OMDemiurgeSize::pixels(std::max(1.0f, viewWidth)))
            ->style("height", OMDemiurgeSize::pixels(thickness))
            ->style("offsetX", OMDemiurgeSize::pixels(0.0f))
            ->style("offsetY", OMDemiurgeSize::pixels(std::max(0.0f, paneHeight - thickness)));
        horizontalBar->setMetrics(viewWidth, contentWidth);
        horizontalBar->setScrollOffset(visualX, false);

        const float rawX = scrollX.get();
        horizontalBar->setVisualOverscroll(rawX < 0.0f ? -rawX : std::max(0.0f, rawX - maxScrollX));
    }

    verticalBar->setDisabled(disabled, false);
    horizontalBar->setDisabled(disabled, false);

    OMDemiurgeContainerNode::update();
}

auto OMDemiurgeScrollPane::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled || button != 1)
    {
        return Ignored;
    }

    pressed = true;
    dragging = false;
    pressX = x;
    pressY = y;
    dragStartX = scrollX.get();
    dragStartY = scrollY.get();
    dragVelocityX = 0.0f;
    dragVelocityY = 0.0f;
    lastDragTime = std::chrono::steady_clock::now();
    captureMouse();
    return Ignored;
}

auto OMDemiurgeScrollPane::updateDrag(float x, float y) -> OMDemiurgeEventResult
{
    const bool vertical = orientation != OMDemiurgeScrollPaneOrientation::Horizontal;
    const bool horizontal = orientation != OMDemiurgeScrollPaneOrientation::Vertical;
    const float threshold = std::max(1.0f, stylesStorage.get<float>("drag_threshold", 4.0f));
    const float dx = x - pressX;
    const float dy = y - pressY;

    if (!dragging)
    {
        if ((horizontal && std::abs(dx) > threshold) || (vertical && std::abs(dy) > threshold))
        {
            dragging = true;
        }
        else
        {
            return Ignored;
        }
    }

    const float oldX = scrollX.get();
    const float oldY = scrollY.get();
    float nextX = oldX;
    float nextY = oldY;

    if (horizontal && maxScrollX > 0.0f)
    {
        nextX = dragStartX - dx;
    }

    if (vertical && maxScrollY > 0.0f)
    {
        nextY = dragStartY - dy;
    }

    const auto now = std::chrono::steady_clock::now();
    const float dt = std::chrono::duration<float>(now - lastDragTime).count();

    if (dt > 0.001f)
    {
        dragVelocityX = std::clamp((nextX - oldX) / dt, -6000.0f, 6000.0f);
        dragVelocityY = std::clamp((nextY - oldY) / dt, -6000.0f, 6000.0f);
        lastDragTime = now;
    }

    scrollX.setTo(nextX);
    scrollY.setTo(nextY);

    notifyScroll();
    return Handled;
}

auto OMDemiurgeScrollPane::processMouseMove(float x, float y) -> OMDemiurgeEventResult
{
    if (!pressed || disabled)
    {
        return Ignored;
    }

    // A child (button/slider/...) may have taken the capture; in that case the
    // press does not belong to the pane anymore (used to cancel stale presses).
    if (!hasMouseCapture())
    {
        pressed = false;
        return Ignored;
    }

    return updateDrag(x, y);
}

auto OMDemiurgeScrollPane::processNestedDrag(const std::shared_ptr<OMDemiurgeNode> &captured,
                                             const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
{
    if (disabled || event.type != OMDemiurgeEventType::MouseMove || !pressed)
    {
        return Ignored;
    }

    // A focused control (slider, switch, input, ...) is being manipulated: its
    // drag must not turn into a pane scroll.
    if (captured && captured->hasFocus())
    {
        return Ignored;
    }

    // A ScrollBar drag (including this pane's own bars or a bar embedded in the
    // content) owns the mouse capture; do not steal it.
    if (captured && dynamic_cast<OMDemiurgeScrollBar *>(captured.get()) != nullptr)
    {
        return Ignored;
    }

    const auto result = updateDrag(event.x, event.y);

    if (result == Handled && !hasMouseCapture())
    {
        // The press started on a child that captured the mouse; take the capture
        // over as soon as the drag threshold is crossed.
        captureMouse();
    }

    return result;
}

auto OMDemiurgeScrollPane::processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (!pressed || button != 1)
    {
        return Ignored;
    }

    const bool wasDragging = dragging;
    pressed = false;
    dragging = false;
    releaseMouse();

    if (wasDragging)
    {
        const bool outside = scrollX.get() < 0.0f || scrollX.get() > maxScrollX || scrollY.get() < 0.0f ||
                             scrollY.get() > maxScrollY;

        if (outside)
        {
            applyBounce();
        }
        else
        {
            applyInertia();
        }

        notifyScroll();
        return Handled;
    }

    return Ignored;
}

auto OMDemiurgeScrollPane::processMouseScroll(float x, float y, float wx, float wy) -> OMDemiurgeEventResult
{
    if (disabled || !containsPoint(x, y))
    {
        return Ignored;
    }

    const bool vertical = orientation != OMDemiurgeScrollPaneOrientation::Horizontal;
    const bool horizontal = orientation != OMDemiurgeScrollPaneOrientation::Vertical;
    const float step = std::max(1.0f, stylesStorage.get<float>("wheel_step", 40.0f));
    const float speed = std::max(0.01f, stylesStorage.get<float>("wheel_speed", 0.18f));
    bool consumed = false;

    if (vertical && maxScrollY > 0.0f && wy != 0.0f)
    {
        const float current = scrollY.get();
        const float target = std::clamp(current - wy * step, 0.0f, maxScrollY);

        const bool atEdge = target <= 0.01f || target >= maxScrollY - 0.01f;

        if (std::abs(target - current) < 0.5f || (atEdge && std::abs(target - current) < 2.0f))
        {
            // Already at/near the edge: snap instead of restarting an animation,
            // so repeated wheel events cannot produce a sub-pixel shake.
            scrollY.setTo(target);
        }
        else
        {
            scrollY.animateTo(target, easeOutCubic<float>, speed);
        }

        consumed = true;
    }

    if (horizontal && maxScrollX > 0.0f && wx != 0.0f)
    {
        const float current = scrollX.get();
        const float target = std::clamp(current - wx * step, 0.0f, maxScrollX);

        const bool atEdge = target <= 0.01f || target >= maxScrollX - 0.01f;

        if (std::abs(target - current) < 0.5f || (atEdge && std::abs(target - current) < 2.0f))
        {
            scrollX.setTo(target);
        }
        else
        {
            scrollX.animateTo(target, easeOutCubic<float>, speed);
        }

        consumed = true;
    }

    if (consumed)
    {
        notifyScroll();
        return Handled;
    }

    return Ignored;
}

void OMDemiurgeScrollPane::setDisabled(bool value, bool animate)
{
    if (disabled == value)
    {
        return;
    }

    disabled = value;

    if (animate)
    {
        disabledAmount.animateTo(value ? 1.0f : 0.0f, easeOutCirc<float>,
                                 stylesStorage.get<float>("animation_speed", 0.18f));
    }
    else
    {
        disabledAmount.setTo(value ? 1.0f : 0.0f);
    }
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
