#ifndef OM_DEMIURGE_ELEMENT_RECT_CHANNEL_HPP
#define OM_DEMIURGE_ELEMENT_RECT_CHANNEL_HPP

#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_quad_channel.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_rect.hpp"
#include <functional>

namespace openminecraft::renderer::common::demiurge::element
{
class OMDemiurgeRectChannel : public OMDemiurgeQuadChannel<OMDemiurgeElementRect>
{
  public:
    OMDemiurgeRectChannel(OMRenderer *renderer, std::function<void()> f) : OMDemiurgeQuadChannel(renderer, f)
    {
    }
    ~OMDemiurgeRectChannel() = default;

    void init(OMRendererBuffer *uniform, OMRendererRenderTarget *target) override;
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
