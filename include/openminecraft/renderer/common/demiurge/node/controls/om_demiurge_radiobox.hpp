#ifndef OM_DEMIURGE_RADIOBOX_HPP
#define OM_DEMIURGE_RADIOBOX_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_control.hpp"
#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace openminecraft::renderer::common::demiurge::node::controls
{
class OMDemiurgeRadioBox;

// Radio group: lays the radios out (row/column) and owns the selection policy.
class OMDemiurgeRadioGroup : public OMDemiurgeContainerNode
{
  public:
    OMDemiurgeRadioGroup();

    void add(const std::shared_ptr<OMDemiurgeRadioBox> &box);

    // Single-select groups clear the previous member; multi-select groups keep them.
    void select(const std::shared_ptr<OMDemiurgeRadioBox> &box);

    // Click policy: single selects, multi toggles.
    void click(const std::shared_ptr<OMDemiurgeRadioBox> &box);

    void setDirection(OMDemiurgeDirection direction);
    void setGap(float pixels);
    void setMultiSelect(bool value);
    auto isMultiSelect() const -> bool
    {
        return multiSelect;
    }

  private:
    std::vector<std::weak_ptr<OMDemiurgeRadioBox>> members;
    bool multiSelect = false;
};

// Radio button: circular box + animated inner dot + optional label.
class OMDemiurgeRadioBox : public OMDemiurgeContainerNode, public OMDemiurgeControl
{
  public:
    explicit OMDemiurgeRadioBox(geom::OMFontSet *fontset = nullptr);
    ~OMDemiurgeRadioBox() override;

    auto submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void override;
    void update() override;

    auto processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseEnter(float x, float y) -> OMDemiurgeEventResult override;
    auto processMouseExit(float x, float y) -> OMDemiurgeEventResult override;

    // Selecting through this call also clears the rest of the attached group.
    void setSelected(bool value, bool animate = true);
    auto isSelected() const -> bool
    {
        return selected;
    }
    void setGroup(std::shared_ptr<OMDemiurgeRadioGroup> value);

    // Fan-out used by the group to clear siblings without re-notifying them.
    void applySelection(bool value, bool animate, bool notify);

    void setOnChange(std::function<void(bool)> handler);
    void setDotColor(glm::vec4 color);
    void setBoxColors(glm::vec4 unselectedColor, glm::vec4 selectedColor);
    void setTextColor(glm::vec4 color);
    void setLabel(const std::string &label);
    void setAnimationSpeed(float seconds);
    void setDisabled(bool value, bool animate = true) override;
    auto isDisabled() const -> bool
    {
        return disabled;
    }

    animation::OMAnimationValue<float> selectAmount;
    animation::OMAnimationValue<glm::vec4> boxUnselected;
    animation::OMAnimationValue<glm::vec4> boxSelected;
    animation::OMAnimationValue<glm::vec4> dotColor;
    animation::OMAnimationValue<glm::vec4> labelColor;
    animation::OMAnimationValue<float> hoverAmount;
    animation::OMAnimationValue<float> pressAmount;
    animation::OMAnimationValue<float> disabledAmount;

  private:
    std::shared_ptr<OMDemiurgeNode> boxNode;
    std::shared_ptr<OMDemiurgeNode> dotNode;
    std::shared_ptr<OMDemiurgeNode> textNode;

    bool selected = false;
    bool disabled = false;
    std::shared_ptr<OMDemiurgeRadioGroup> group;
    std::function<void(bool)> handle = [](bool) {};
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
