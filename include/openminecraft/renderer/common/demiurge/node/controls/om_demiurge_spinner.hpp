#ifndef OM_DEMIURGE_SPINNER_HPP
#define OM_DEMIURGE_SPINNER_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <memory>
#include <vector>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Indeterminate circular spinner: a ring of dots fading around the circle.
class OMDemiurgeSpinner : public OMDemiurgeContainerNode
{
  public:
    OMDemiurgeSpinner();
    ~OMDemiurgeSpinner() override;

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    void update() override;

    void setColor(glm::vec4 color);
    void setSpeed(float radiansPerSecond);
    void setDotCount(int count);
    void setDisabled(bool value, bool animate = true);
    auto isDisabled() const -> bool
    {
        return disabled;
    }

    animation::OMAnimationValue<glm::vec4> dotColor;
    animation::OMAnimationValue<float> disabledAmount;

  private:
    void rebuildDots();

    std::vector<std::shared_ptr<OMDemiurgeNode>> dots;
    float spinSpeed = 4.0f;
    int dotCount = 8;
    bool disabled = false;
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
