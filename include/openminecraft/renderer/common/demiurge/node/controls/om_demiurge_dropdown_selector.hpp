#ifndef OM_DEMIURGE_DROPDOWN_SELECTOR_HPP
#define OM_DEMIURGE_DROPDOWN_SELECTOR_HPP

#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_control.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_event.hpp"
#include <chrono>
#include <functional>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Drop-down selector. The header stays in the normal layout; the option list is
// mounted as an overlay subtree on the root, so it escapes ScrollPane clips and
// is rendered after all normal layers.
class OMDemiurgeDropDownSelector : public OMDemiurgeContainerNode, public OMDemiurgeControl
{
  public:
    explicit OMDemiurgeDropDownSelector(geom::OMFontSet *fontset = nullptr);
    ~OMDemiurgeDropDownSelector() override;

    auto processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseEnter(float x, float y) -> OMDemiurgeEventResult override;
    auto processMouseExit(float x, float y) -> OMDemiurgeEventResult override;
    void update() override;
    void remove() override;

    void setItems(const std::vector<std::string> &values);
    auto getItems() const -> const std::vector<std::string> &
    {
        return items;
    }

    void setSelectedIndex(int index, bool notify = true);
    auto getSelectedIndex() const -> int;

    auto getSelectedValue() const -> std::string;

    void setPlaceholder(const std::string &value);
    void setHeaderColors(const glm::vec4 &normal, const glm::vec4 &hovered, const glm::vec4 &pressed);
    void setOnSelect(std::function<void(int, const std::string &)> handler);

    void open();
    void close();
    auto isOpen() const -> bool
    {
        return opened.load();
    }

    void setDisabled(bool value, bool animate = true) override;
    auto isDisabled() const -> bool
    {
        return disabled.load();
    }

  private:
    void ensurePopup();
    auto computePanelRect(const OMDemiurgeRect &bound, const OMDemiurgeRect &viewport) -> OMDemiurgeRect;
    void rebuildPopupContent();
    void selectItem(int index);

    std::shared_ptr<OMDemiurgeNode> background;
    std::shared_ptr<OMDemiurgeNode> labelNode;
    std::shared_ptr<OMDemiurgeNode> arrowNode;

    std::shared_ptr<OMDemiurgeNode> popupLayer;
    std::shared_ptr<OMDemiurgeNode> panel;
    std::shared_ptr<OMDemiurgeNode> backdrop;
    std::shared_ptr<OMDemiurgeNode> scrollPane;
    std::shared_ptr<OMDemiurgeNode> scrollContent;
    std::vector<std::shared_ptr<OMDemiurgeNode>> itemNodes;
    std::weak_ptr<OMDemiurgeNode> popupParent;

    geom::OMFontSet *fontset = nullptr;
    std::vector<std::string> items;
    int selectedIndex = -1;
    std::atomic<bool> opened{false};
    bool closing = false;
    bool pendingScrollToSelection = false;
    float lastPopupScrollY = -1.0f;
    float popupHeight = 44.0f;
    float lastPopupOriginX = 0.0f;
    float lastPopupOriginY = 0.0f;
    float lastPopupViewWidth = 0.0f;
    float lastPopupViewHeight = 0.0f;
    std::atomic<bool> disabled{false};
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>>(true);
    mutable std::recursive_mutex stateMutex;
    std::string placeholder = "Select...";

    animation::OMAnimationValue<float> hoverAmount{0.0f};
    animation::OMAnimationValue<float> pressAmount{0.0f};
    glm::vec4 normalColor{0.16f, 0.16f, 0.20f, 1.0f};
    glm::vec4 hoverColor{0.21f, 0.21f, 0.27f, 1.0f};
    glm::vec4 pressedColor{0.28f, 0.28f, 0.36f, 1.0f};
    bool pressing = false;
    std::chrono::steady_clock::time_point pressStarted;
    std::function<void(int, const std::string &)> onSelect;
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
