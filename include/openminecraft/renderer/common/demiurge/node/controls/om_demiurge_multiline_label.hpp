#ifndef OM_DEMIURGE_MULTILINE_LABEL_HPP
#define OM_DEMIURGE_MULTILINE_LABEL_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf_multiline.hpp"
#include <string>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Public multi-line label wrapper around OMDemiurgeTextSdfMultilineNode.
// '\n' starts a new line; text and color are exposed as APIs.
class OMDemiurgeMultilineLabel : public node::OMDemiurgeTextSdfMultilineNode
{
  public:
    explicit OMDemiurgeMultilineLabel(geom::OMFontSet *fontset = nullptr);
    ~OMDemiurgeMultilineLabel() override = default;

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
