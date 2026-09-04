#ifndef OM_DEMIURGE_BUTTON_HPP
#define OM_DEMIURGE_BUTTON_HPP

#include "openminecraft/fontproc/om_fontset.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
namespace openminecraft::renderer::common::demiurge::node::controls
{
class OMDemiurgeButton : public OMDemiurgeContainerNode
{
  public:
    OMDemiurgeButton(fontproc::OMFontSet *fontset);
    ~OMDemiurgeButton() override;

    auto processEvent(float x, float y, OMDemiurgeEventType type, uint8_t ext, void *data)
        -> OMDemiurgeEventResult override;
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
