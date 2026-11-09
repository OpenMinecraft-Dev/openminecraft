#ifndef OM_DEMIURGE_CHECKBOX_HPP
#define OM_DEMIURGE_CHECKBOX_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_control.hpp"
#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <functional>
#include <memory>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Checkbox control: rounded box + SVG check mark + optional label.
class OMDemiurgeCheckBox : public OMDemiurgeContainerNode, public OMDemiurgeControl
{
  public:
    explicit OMDemiurgeCheckBox(geom::OMFontSet *fontset = nullptr);
    ~OMDemiurgeCheckBox() override;

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    void update() override;

    auto processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseEnter(float x, float y) -> OMDemiurgeEventResult override;
    auto processMouseExit(float x, float y) -> OMDemiurgeEventResult override;

    void setChecked(bool value, bool animate = true);
    auto isChecked() const -> bool
    {
        return checked;
    }
    void toggle();
    void setOnChange(std::function<void(bool)> handler);
    void setCheckColor(glm::vec4 color);
    void setBoxColors(glm::vec4 uncheckedColor, glm::vec4 checkedColor);
    void setTextColor(glm::vec4 color);
    void setLabel(const std::string &label);
    void setAnimationSpeed(float seconds);
    void setDisabled(bool value, bool animate = true) override;
    auto isDisabled() const -> bool
    {
        return disabled;
    }

    animation::OMAnimationValue<float> checkAmount;
    animation::OMAnimationValue<glm::vec4> boxUnchecked;
    animation::OMAnimationValue<glm::vec4> boxChecked;
    animation::OMAnimationValue<glm::vec4> checkColor;
    animation::OMAnimationValue<glm::vec4> labelColor;
    animation::OMAnimationValue<float> hoverAmount;
    animation::OMAnimationValue<float> pressAmount;
    animation::OMAnimationValue<float> disabledAmount;

  private:
    std::shared_ptr<OMDemiurgeNode> boxNode;
    std::shared_ptr<OMDemiurgeNode> checkNode;
    std::shared_ptr<OMDemiurgeNode> textNode;

    bool checked = false;
    bool disabled = false;
    std::function<void(bool)> handle = [](bool) {};
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
