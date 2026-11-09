#ifndef OM_DEMIURGE_DIVIDER_HPP
#define OM_DEMIURGE_DIVIDER_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <memory>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Thin separator line, horizontal (default) or vertical.
class OMDemiurgeDivider : public OMDemiurgeContainerNode
{
  public:
    explicit OMDemiurgeDivider(bool vertical = false);
    ~OMDemiurgeDivider() override;

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    void update() override;

    void setColor(glm::vec4 color);
    void setThickness(float pixels);

    animation::OMAnimationValue<glm::vec4> lineColor;

  private:
    std::shared_ptr<OMDemiurgeNode> lineNode;
    bool vertical = false;
    float thickness = 1.0f;
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
