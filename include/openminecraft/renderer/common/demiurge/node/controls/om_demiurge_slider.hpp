#ifndef OM_DEMIURGE_SLIDER_HPP
#define OM_DEMIURGE_SLIDER_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_control.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <functional>
#include <memory>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Horizontal slider with draggable thumb. Value is normalized to 0..1.
class OMDemiurgeSlider : public OMDemiurgeContainerNode, public OMDemiurgeControl
{
  public:
    OMDemiurgeSlider();
    ~OMDemiurgeSlider() override;

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    void update() override;

    auto processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseMove(float x, float y) -> OMDemiurgeEventResult override;
    auto processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseEnter(float x, float y) -> OMDemiurgeEventResult override;
    auto processMouseExit(float x, float y) -> OMDemiurgeEventResult override;

    void setValue(float value, bool notify = true);
    auto getValue() const -> float
    {
        return value;
    }
    void setOnChange(std::function<void(float)> handler);
    void setColors(glm::vec4 track, glm::vec4 fill, glm::vec4 thumb);
    void setAnimationSpeed(float seconds);
    void setDisabled(bool value, bool animate = true) override;
    auto isDisabled() const -> bool
    {
        return disabled;
    }

    animation::OMAnimationValue<float> hoverAmount;
    animation::OMAnimationValue<float> pressAmount;
    animation::OMAnimationValue<float> disabledAmount;
    animation::OMAnimationValue<glm::vec4> trackColor;
    animation::OMAnimationValue<glm::vec4> fillColor;
    animation::OMAnimationValue<glm::vec4> thumbColor;

  private:
    auto updateFromMouse(float x) -> bool;

    std::shared_ptr<OMDemiurgeNode> trackNode;
    std::shared_ptr<OMDemiurgeNode> fillNode;
    std::shared_ptr<OMDemiurgeNode> thumbNode;

    float value = 0.0f;
    bool dragging = false;
    bool disabled = false;
    std::function<void(float)> handle = [](float) {};
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
