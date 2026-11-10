#ifndef OM_DEMIURGE_SCROLLBAR_HPP
#define OM_DEMIURGE_SCROLLBAR_HPP

#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_control.hpp"
#include <functional>
#include <memory>

namespace openminecraft::renderer::common::demiurge::node::controls
{
enum class OMDemiurgeScrollBarOrientation
{
    Vertical,
    Horizontal,
};

// Scroll bar driven by externally supplied viewport/content metrics. It computes
// the thumb size/position from viewSize/contentSize, and reports drag/wheel
// changes through setOnScroll so a future ScrollPane can keep its content in sync.
// Appearance follows the tab indicator: a thin rounded pill.
class OMDemiurgeScrollBar : public OMDemiurgeContainerNode, public OMDemiurgeControl
{
  public:
    OMDemiurgeScrollBar();
    ~OMDemiurgeScrollBar() override;

    void update() override;
    auto processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseMove(float x, float y) -> OMDemiurgeEventResult override;
    auto processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseScroll(float x, float y, float wx, float wy) -> OMDemiurgeEventResult override;

    void setOrientation(OMDemiurgeScrollBarOrientation value);
    auto getOrientation() const -> OMDemiurgeScrollBarOrientation
    {
        return orientation;
    }

    // viewSize: visible area size along the scroll axis.
    // contentSize: total content size along the scroll axis.
    void setMetrics(float viewSize, float contentSize);
    void setViewSize(float value);
    void setContentSize(float value);
    auto getViewSize() const -> float
    {
        return viewSize;
    }
    auto getContentSize() const -> float
    {
        return contentSize;
    }

    // When overscroll is allowed, values outside [0, contentSize - viewSize] are
    // kept and visualized by compressing the thumb. When disabled the offset is
    // clamped instead.
    void setAllowOverscroll(bool value);
    // Extra overscroll used only for the thumb compression animation. Lets a
    // ScrollPane show rubber-band feedback while the bar's own offset is clamped.
    void setVisualOverscroll(float value);
    auto getVisualOverscroll() const -> float
    {
        return visualOverscroll;
    }
    auto isOverscrollAllowed() const -> bool
    {
        return allowOverscroll;
    }
    void setScrollOffset(float value, bool notify = false);
    auto getScrollOffset() const -> float
    {
        return scrollOffset;
    }
    auto getMaxScrollOffset() const -> float;
    auto getThumbLength() const -> float
    {
        return visualThumbLength;
    }
    auto getOverscroll() const -> float;
    auto isOverscrolling() const -> bool
    {
        return getOverscroll() > 0.0f;
    }

    void setOnScroll(std::function<void(float)> handler);
    void setDisabled(bool value, bool animate = true) override;
    auto isDisabled() const -> bool
    {
        return disabled;
    }

    animation::OMAnimationValue<float> disabledAmount;

  private:
    std::shared_ptr<OMDemiurgeNode> trackNode;
    std::shared_ptr<OMDemiurgeNode> thumbNode;

    OMDemiurgeScrollBarOrientation orientation = OMDemiurgeScrollBarOrientation::Vertical;
    float viewSize = 0.0f;
    float contentSize = 0.0f;
    float scrollOffset = 0.0f;
    float thumbLength = 0.0f;
    float visualThumbLength = 0.0f;
    float trackLength = 0.0f;

    bool dragging = false;
    float dragStartPointer = 0.0f;
    float dragStartOffset = 0.0f;
    bool disabled = false;
    bool allowOverscroll = true;
    float visualOverscroll = 0.0f;
    std::function<void(float)> handleScroll = [](float) {};

    auto axisCoordinate(float x, float y) const -> float;
    auto thumbTravel() const -> float;
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
