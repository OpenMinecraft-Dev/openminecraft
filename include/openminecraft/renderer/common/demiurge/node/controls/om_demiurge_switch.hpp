#ifndef OM_DEMIURGE_SWITCH_HPP
#define OM_DEMIURGE_SWITCH_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <functional>
#include <memory>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// iOS/macOS style switch: rounded track with a sliding thumb.
class OMDemiurgeSwitch : public OMDemiurgeContainerNode
{
  public:
    OMDemiurgeSwitch();
    ~OMDemiurgeSwitch() override;

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
    void setOnChange(std::function<void(bool)> handler);
    void setTrackColors(glm::vec4 offColor, glm::vec4 onColor);
    void setThumbColor(glm::vec4 color);
    void setAnimationSpeed(float seconds);
    void setDisabled(bool value, bool animate = true);
    auto isDisabled() const -> bool
    {
        return disabled;
    }

    animation::OMAnimationValue<float> onAmount;
    animation::OMAnimationValue<float> hoverAmount;
    animation::OMAnimationValue<float> pressAmount;
    animation::OMAnimationValue<float> disabledAmount;
    animation::OMAnimationValue<glm::vec4> trackOff;
    animation::OMAnimationValue<glm::vec4> trackOn;
    animation::OMAnimationValue<glm::vec4> thumbColor;

  private:
    std::shared_ptr<OMDemiurgeNode> trackNode;
    std::shared_ptr<OMDemiurgeNode> thumbNode;

    bool checked = false;
    bool disabled = false;
    std::function<void(bool)> handle = [](bool) {};
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
