#ifndef OM_DEMIURGE_CONTAINER_HPP
#define OM_DEMIURGE_CONTAINER_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
namespace openminecraft::renderer::common::demiurge::node
{
class OMDemiurgeContainerNode : public OMDemiurgeNode
{
  public:
    OMDemiurgeContainerNode();
    virtual ~OMDemiurgeContainerNode();

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    auto remove() -> void override;
};
} // namespace openminecraft::renderer::common::demiurge::node

#endif
