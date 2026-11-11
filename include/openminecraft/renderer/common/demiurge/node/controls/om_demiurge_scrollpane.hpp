#ifndef OM_DEMIURGE_SCROLLPANE_HPP
#define OM_DEMIURGE_SCROLLPANE_HPP

#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_control.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_scrollbar.hpp"
#include <chrono>
#include <functional>
#include <memory>

namespace openminecraft::renderer::common::demiurge::node::controls
{
enum class OMDemiurgeScrollPaneOrientation
{
    Horizontal,
    Vertical,
    Both,
};

// Scrollable viewport with optional horizontal/vertical scroll bars.
//
// Layout rules:
// - Vertical: content width is constrained to the viewport when stretch is on;
//   content height is free (scrolls vertically).
// - Horizontal: content height is constrained to the viewport when stretch is
//   on; content width is free (scrolls horizontally).
// - Both: content width and height are free.
//
// Dragging the content beyond its bounds applies a rubber-band resistance and
// springs back on release. The internal ScrollBars are two-way bound to the
// content offset.
class OMDemiurgeScrollPane : public OMDemiurgeContainerNode, public OMDemiurgeControl
{
  public:
    OMDemiurgeScrollPane();
    ~OMDemiurgeScrollPane() override;

    void update() override;
    auto processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processNestedDrag(const std::shared_ptr<OMDemiurgeNode> &captured,
                           const OMDemiurgeEvent &event) -> OMDemiurgeEventResult override;
    auto processMouseMove(float x, float y) -> OMDemiurgeEventResult override;
    auto processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseScroll(float x, float y, float wx, float wy) -> OMDemiurgeEventResult override;

    void setOrientation(OMDemiurgeScrollPaneOrientation value);
    auto getOrientation() const -> OMDemiurgeScrollPaneOrientation
    {
        return orientation;
    }

    void setContentNode(const std::shared_ptr<OMDemiurgeNode> &content);
    auto getContentNode() const -> std::shared_ptr<OMDemiurgeNode>
    {
        return contentNode;
    }

    void setContentStretch(bool value);
    auto isContentStretch() const -> bool
    {
        return contentStretch;
    }

    // Raw offsets; may temporarily exceed the valid range while dragging.
    void setScrollOffset(float x, float y, bool notify = false);
    auto getScrollX() -> float;
    auto getScrollY() -> float;

    void setOnScroll(std::function<void(float, float)> handler);
    void setDisabled(bool value, bool animate = true) override;
    auto isDisabled() const -> bool
    {
        return disabled;
    }

    auto fetchVerticalScrollBar() const -> std::shared_ptr<OMDemiurgeScrollBar>
    {
        return verticalBar;
    }
    auto fetchHorizontalScrollBar() const -> std::shared_ptr<OMDemiurgeScrollBar>
    {
        return horizontalBar;
    }

    animation::OMAnimationValue<float> disabledAmount;

  private:
    std::shared_ptr<OMDemiurgeNode> contentHost;
    std::shared_ptr<OMDemiurgeNode> viewportClip;
    std::shared_ptr<OMDemiurgeNode> contentNode;
    std::shared_ptr<OMDemiurgeScrollBar> verticalBar;
    std::shared_ptr<OMDemiurgeScrollBar> horizontalBar;

    OMDemiurgeScrollPaneOrientation orientation = OMDemiurgeScrollPaneOrientation::Vertical;
    bool contentStretch = true;
    bool disabled = false;

    animation::OMAnimationValue<float> scrollX{0.0f};
    animation::OMAnimationValue<float> scrollY{0.0f};

    bool pressed = false;
    bool dragging = false;
    float pressX = 0.0f;
    float pressY = 0.0f;
    float dragStartX = 0.0f;
    float dragStartY = 0.0f;
    float dragVelocityX = 0.0f;
    float dragVelocityY = 0.0f;
    std::chrono::steady_clock::time_point lastDragTime;

    float viewWidth = 0.0f;
    float viewHeight = 0.0f;
    float contentWidth = 0.0f;
    float contentHeight = 0.0f;
    float maxScrollX = 0.0f;
    float maxScrollY = 0.0f;
    float lastMaxScrollX = -1.0f;
    float lastMaxScrollY = -1.0f;

    std::function<void(float, float)> handleScroll = [](float, float) {};

    auto rubberBand(float raw, float maxValue) -> float;
    auto updateDrag(float x, float y) -> OMDemiurgeEventResult;
    void applyBounce();
    void applyInertia();
    void preserveScrollOnContentChange();
    void notifyScroll();
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
