#ifndef OM_DEMIURGE_LINK_HPP
#define OM_DEMIURGE_LINK_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_control.hpp"
#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <functional>
#include <memory>
#include <string>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Clickable text with an underline that appears on hover.
class OMDemiurgeLink : public OMDemiurgeContainerNode, public OMDemiurgeControl
{
  public:
    explicit OMDemiurgeLink(geom::OMFontSet *fontset = nullptr);
    ~OMDemiurgeLink() override;

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    void update() override;

    auto processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseEnter(float x, float y) -> OMDemiurgeEventResult override;
    auto processMouseExit(float x, float y) -> OMDemiurgeEventResult override;

    void setText(const std::string &text);
    void setTextColor(glm::vec4 color);
    void setUnderlineColor(glm::vec4 color);
    void setOnClick(std::function<void()> handler);
    void setDisabled(bool value, bool animate = true) override;
    auto isDisabled() const -> bool
    {
        return disabled;
    }

    animation::OMAnimationValue<float> hoverAmount;
    animation::OMAnimationValue<float> pressAmount;
    animation::OMAnimationValue<float> disabledAmount;
    animation::OMAnimationValue<glm::vec4> textColor;
    animation::OMAnimationValue<glm::vec4> underlineColor;

  private:
    std::shared_ptr<OMDemiurgeNode> textNode;
    std::shared_ptr<OMDemiurgeNode> underlineNode;
    bool disabled = false;
    std::function<void()> handle = []() {};
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
