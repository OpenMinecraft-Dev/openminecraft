#ifndef OM_DEMIURGE_ELEMENT_CLIPRECT_CHANNEL_HPP
#define OM_DEMIURGE_ELEMENT_CLIPRECT_CHANNEL_HPP

#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_cliprect.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_quad_channel.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <functional>

namespace openminecraft::renderer::common::demiurge::element
{
class OMDemiurgeClipRectChannel : public OMDemiurgeQuadChannel<OMDemiurgeElementClipRect>
{
  public:
    OMDemiurgeClipRectChannel(OMRenderer *renderer, std::function<void()> f) : OMDemiurgeQuadChannel(renderer, f)
    {
    }
    ~OMDemiurgeClipRectChannel() = default;

    void init(OMRendererBuffer *uniform, OMRendererRenderTarget *target) override;
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
