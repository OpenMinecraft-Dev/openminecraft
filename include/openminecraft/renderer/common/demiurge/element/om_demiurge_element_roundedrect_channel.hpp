#ifndef OM_DEMIURGE_ELEMENT_ROUNDEDRECT_CHANNEL_HPP
#define OM_DEMIURGE_ELEMENT_ROUNDEDRECT_CHANNEL_HPP

#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_quad_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <functional>
namespace openminecraft::renderer::common::demiurge::element
{
class OMDemiurgeRoundedRectChannel : public OMDemiurgeQuadChannel<OMDemiurgeElementRoundedRect>
{
  public:
    OMDemiurgeRoundedRectChannel(OMRenderer *renderer, std::function<void()> f) : OMDemiurgeQuadChannel(renderer, f)
    {
    }
    ~OMDemiurgeRoundedRectChannel() = default;

    void init(OMRendererBuffer *uniform, OMRendererRenderTarget *target) override;
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
