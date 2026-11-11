#ifndef OM_DEMIURGE_CONTAINER_HPP
#define OM_DEMIURGE_CONTAINER_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
namespace openminecraft::renderer::common::demiurge::node::container
{
class OMDemiurgeContainerNode : public OMDemiurgeNode
{
  public:
    OMDemiurgeContainerNode();
    virtual ~OMDemiurgeContainerNode();

    auto submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void override;
    auto remove() -> void override;
};
} // namespace openminecraft::renderer::common::demiurge::node::container

namespace openminecraft::renderer::common::demiurge::node
{
// Compatibility alias: container nodes live in the node::container package.
using container::OMDemiurgeContainerNode;
} // namespace openminecraft::renderer::common::demiurge::node

#endif
