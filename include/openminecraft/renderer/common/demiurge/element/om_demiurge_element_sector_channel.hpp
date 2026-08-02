#ifndef OM_DEMIURGE_ELEMENT_SECTOR_CHANNEL_HPP
#define OM_DEMIURGE_ELEMENT_SECTOR_CHANNEL_HPP

#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_quad_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_sector.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
class OMDemiurgeSectorChannel : public OMDemiurgeQuadChannel<OMDemiurgeElementSector>
{
  public:
    OMDemiurgeSectorChannel(OMRenderer *renderer, std::function<void()> f) : OMDemiurgeQuadChannel(renderer, f)
    {
    }
    ~OMDemiurgeSectorChannel() = default;

    void init(OMRendererBuffer *uniform, OMRendererRenderTarget *target) override;
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
