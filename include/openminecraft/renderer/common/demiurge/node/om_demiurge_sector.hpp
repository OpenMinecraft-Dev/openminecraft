#ifndef OM_DEMIURGE_SECTOR_HPP
#define OM_DEMIURGE_SECTOR_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
namespace openminecraft::renderer::common::demiurge::node
{
class OMDemiurgeSectorNode : public OMDemiurgeNode
{
  public:
    OMDemiurgeSectorNode();
    virtual ~OMDemiurgeSectorNode();

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    auto remove() -> void override;

  private:
    int sectorId = -1;
    OMDemiurgeRendererHandler *handler = nullptr;
};
} // namespace openminecraft::renderer::common::demiurge::node

#endif
