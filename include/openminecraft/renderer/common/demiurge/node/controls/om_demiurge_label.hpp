#ifndef OM_DEMIURGE_LABEL_HPP
#define OM_DEMIURGE_LABEL_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include <string>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Public label wrapper around OMDemiurgeTextSdfNode.
//
// Text and color are exposed as APIs instead of requiring callers to write the
// underlying `text` / `color` styles. Color changes are animated; direct style
// writes remain supported as long as setColor() has not been used.
class OMDemiurgeLabel : public node::OMDemiurgeTextSdfNode
{
  public:
    explicit OMDemiurgeLabel(geom::OMFontSet *fontset = nullptr);
    ~OMDemiurgeLabel() override = default;

    void update() override;

    void setText(const std::string &text);
    auto getText() -> std::string;
    void setColor(const glm::vec4 &color);
    auto getColor() -> glm::vec4;
    void setAnimationSpeed(float seconds);

  private:
    animation::OMAnimationValue<glm::vec4> labelColor{glm::vec4(1.0f)};
    glm::vec4 lastColor{glm::vec4(1.0f)};
    bool colorAnimated = false;
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
