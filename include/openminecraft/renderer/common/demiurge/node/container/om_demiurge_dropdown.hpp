#ifndef OM_DEMIURGE_DROPDOWN_HPP
#define OM_DEMIURGE_DROPDOWN_HPP

#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_pane.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_control.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_event.hpp"
#include <functional>
#include <memory>
#include <string>

namespace openminecraft::renderer::common::demiurge::node::container
{
using controls::OMDemiurgeControl;
// Collapsible pane: rounded-rect body, a title row and a rotating chevron on the
// right. Content is provided with setContentNode() and clipped by the pane while
// the height animates between collapsed and expanded.
class OMDemiurgeDropDownPane : public OMDemiurgePane, public OMDemiurgeControl
{
  public:
    explicit OMDemiurgeDropDownPane(geom::OMFontSet *fontset = nullptr);
    ~OMDemiurgeDropDownPane() override;

    auto submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void override;
    void update() override;

    auto processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;

    void setTitle(const std::string &title);
    void setContentNode(const std::shared_ptr<OMDemiurgeNode> &node);
    auto getContentNode() const -> std::shared_ptr<OMDemiurgeNode>
    {
        return content;
    }

    void setExpanded(bool expanded, bool animate = true);
    void toggle(bool animate = true);
    auto isExpanded() const -> bool
    {
        return expanded;
    }

    void setOnToggle(std::function<void(bool)> handler);
    void setHeaderHeight(float pixels);
    void setAnimationSpeed(float seconds);
    void setDisabled(bool value, bool animate = true) override;
    auto isDisabled() const -> bool
    {
        return disabled;
    }

    animation::OMAnimationValue<float> openAmount;
    animation::OMAnimationValue<float> disabledAmount;

  private:
    std::shared_ptr<OMDemiurgeNode> background;
    std::shared_ptr<OMDemiurgeNode> header;
    std::shared_ptr<OMDemiurgeNode> titleNode;
    std::shared_ptr<OMDemiurgeNode> arrowNode;
    std::shared_ptr<OMDemiurgeNode> contentHost;
    std::shared_ptr<OMDemiurgeNode> content;

    geom::OMFontSet *fontset = nullptr;
    bool expanded = false;
    bool disabled = false;
    bool pressed = false;
    float contentHeight = 0.0f;
    float headerHeight = 34.0f;
    float lastHeight = -1.0f;

    std::function<void(bool)> handleToggle = [](bool) {};
};
} // namespace openminecraft::renderer::common::demiurge::node::container




#endif
