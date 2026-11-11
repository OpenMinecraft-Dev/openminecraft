#ifndef OM_DEMIURGE_ICONBUTTON_HPP
#define OM_DEMIURGE_ICONBUTTON_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_control.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <functional>
#include <memory>
#include <string>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Square button with an SVG icon (no label).
class OMDemiurgeIconButton : public OMDemiurgeContainerNode, public OMDemiurgeControl
{
  public:
    explicit OMDemiurgeIconButton(float size = 28.0f);
    ~OMDemiurgeIconButton() override;

    auto submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void override;
    void update() override;

    auto processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseEnter(float x, float y) -> OMDemiurgeEventResult override;
    auto processMouseExit(float x, float y) -> OMDemiurgeEventResult override;

    void setIcon(const std::string &svgPath, float viewBox = 14.0f);
    void setIconColor(glm::vec4 color);
    void setBackgroundColor(glm::vec4 color);
    void setOnClick(std::function<void()> handler);
    void setAnimationSpeed(float seconds);
    void setDisabled(bool value, bool animate = true) override;
    auto isDisabled() const -> bool
    {
        return disabled;
    }

    animation::OMAnimationValue<float> hoverAmount;
    animation::OMAnimationValue<float> pressAmount;
    animation::OMAnimationValue<float> disabledAmount;
    animation::OMAnimationValue<glm::vec4> iconColor;
    animation::OMAnimationValue<glm::vec4> backgroundColor;

  private:
    std::shared_ptr<OMDemiurgeNode> backgroundNode;
    std::shared_ptr<OMDemiurgeNode> iconNode;
    float buttonSize = 28.0f;

    bool disabled = false;
    std::function<void()> handle = []() {};
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
