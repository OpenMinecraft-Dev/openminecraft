#ifndef OM_DEMIURGE_BADGE_HPP
#define OM_DEMIURGE_BADGE_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <memory>
#include <string>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Small rounded label (count/tag).
class OMDemiurgeBadge : public OMDemiurgeRectNode
{
  public:
    explicit OMDemiurgeBadge(geom::OMFontSet *fontset = nullptr);
    ~OMDemiurgeBadge() override;

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    void update() override;

    void setText(const std::string &text);
    void setBackgroundColor(glm::vec4 color);
    void setTextColor(glm::vec4 color);
    void setTextHeight(int height);

    animation::OMAnimationValue<glm::vec4> backgroundColor;
    animation::OMAnimationValue<glm::vec4> textColor;

  private:
    std::shared_ptr<OMDemiurgeNode> textNode;
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
