#ifndef OM_DEMIURGE_BUTTON_HPP
#define OM_DEMIURGE_BUTTON_HPP

#include "glm/fwd.hpp"
#include "openminecraft/fontproc/om_fontset.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <memory>
namespace openminecraft::renderer::common::demiurge::node::controls
{
class OMDemiurgeButton : public OMDemiurgeContainerNode
{
  public:
    OMDemiurgeButton(fontproc::OMFontSet *fontset);
    ~OMDemiurgeButton() override;

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    auto processEvent(float x, float y, OMDemiurgeEventType type, uint8_t ext, void *data)
        -> OMDemiurgeEventResult override;

    void setText(std::string s);
    void setTextColor(int c);
    void setBackgroundColor(int c);
    void setRadius(glm::vec4 r);

  private:
    std::shared_ptr<OMDemiurgeNode> textNode;
    std::shared_ptr<OMDemiurgeNode> bkgNode;
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
