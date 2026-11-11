#ifndef OM_DEMIURGE_TABS_HPP
#define OM_DEMIURGE_TABS_HPP

#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_control.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_event.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace openminecraft::renderer::common::demiurge::node::controls
{
enum class OMDemiurgeTabPosition
{
    Top,
    Bottom,
    Left,
    Right,
};

// Alignment of the tab bar along its own axis: left/right for top/bottom tabs,
// top/bottom for left/right tabs.
enum class OMDemiurgeTabAlignment
{
    Start,
    Center,
    End,
};

// Multi-page container with a tab bar. Every page must be mounted through
// addPage(); only the active page is submitted, while the others stay mounted and
// invisible (no animation for now).
class OMDemiurgeTabMultiPage : public OMDemiurgeContainerNode, public OMDemiurgeControl
{
  public:
    explicit OMDemiurgeTabMultiPage(geom::OMFontSet *fontset = nullptr);
    ~OMDemiurgeTabMultiPage() override;

    auto submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void override;
    void update() override;

    auto processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;

    // Adds a page and mounts it under this control. Returns the page index.
    auto addPage(const std::string &title, const std::shared_ptr<OMDemiurgeNode> &page) -> int;

    void setActivePage(int index);
    auto getActivePage() const -> int
    {
        return activeIndex;
    }
    auto getPageCount() const -> int
    {
        return static_cast<int>(tabs.size());
    }

    void setTabWidth(float pixels);
    // When enabled the page area fills the remaining control height (the control
    // must have an explicit/filled height); pages get height: fill.
    void setPageFill(bool value);
    // Per-page easing used when that page slides in (and the old one slides
    // out). The movement axis follows the tab bar: horizontal for top/bottom,
    // vertical for left/right; direction follows the tab index relationship.
    void setPageEasing(int index, animation::OMEasingFunc easing);
    void setPageAnimationSpeed(float seconds);
    auto getPageAnimationSpeed() const -> float
    {
        return pageAnimationSpeed;
    }
    auto isPageFill() const -> bool
    {
        return pageFill;
    }
    void setTabPosition(OMDemiurgeTabPosition position);
    auto getTabPosition() const -> OMDemiurgeTabPosition
    {
        return tabPosition;
    }

    void setTabAlignment(OMDemiurgeTabAlignment alignment);
    auto getTabAlignment() const -> OMDemiurgeTabAlignment
    {
        return tabAlignment;
    }

    void setOnPageChange(std::function<void(int)> handler);
    void setDisabled(bool value, bool animate = true) override;
    auto isDisabled() const -> bool
    {
        return disabled;
    }

    animation::OMAnimationValue<float> disabledAmount;
    animation::OMAnimationValue<float> indicatorPosition;

  private:
    struct Tab
    {
        std::shared_ptr<OMDemiurgeNode> root;
        std::shared_ptr<OMDemiurgeNode> label;
        std::shared_ptr<OMDemiurgeNode> page;
        std::string title;
        animation::OMEasingFunc easing;
    };

    auto setPageVisible(int index, bool visible) -> void;
    auto finishPageTransition() -> void;
    auto setPageOffset(const std::shared_ptr<OMDemiurgeNode> &page, float x, float y) -> void;
    auto applyActivePage() -> void;

    std::shared_ptr<OMDemiurgeNode> tabBar;
    std::shared_ptr<OMDemiurgeNode> barLine;
    std::shared_ptr<OMDemiurgeNode> pageHost;
    std::shared_ptr<OMDemiurgeNode> pageClip;
    std::shared_ptr<OMDemiurgeNode> indicator;
    std::vector<Tab> tabs;

    geom::OMFontSet *fontset = nullptr;
    int activeIndex = -1;
    OMDemiurgeTabPosition tabPosition = OMDemiurgeTabPosition::Top;
    OMDemiurgeTabAlignment tabAlignment = OMDemiurgeTabAlignment::Center;
    bool pageFill = false;
    std::shared_ptr<OMDemiurgeNode> outgoingPage;
    int outgoingIndex = -1;
    bool pageTransitioning = false;
    float pageTransitionDir = 1.0f;
    float pageAnimationSpeed = 0.25f;
    animation::OMAnimationValue<float> pageProgress{1.0f};
    bool disabled = false;
    float lastHeight = -1.0f;
    std::function<void(int)> handlePageChange = [](int) {};
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
