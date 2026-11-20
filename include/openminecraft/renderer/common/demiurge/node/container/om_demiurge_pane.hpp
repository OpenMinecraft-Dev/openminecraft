#ifndef OM_DEMIURGE_PANE_HPP
#define OM_DEMIURGE_PANE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
namespace openminecraft::renderer::common::demiurge::node::container
{
class OMDemiurgePane : public OMDemiurgeNode
{
  public:
    OMDemiurgePane();
    virtual ~OMDemiurgePane();

    auto submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void override;
    auto remove() -> void override;
};
} // namespace openminecraft::renderer::common::demiurge::node::container

namespace openminecraft::renderer::common::demiurge::node
{
using container::OMDemiurgePane;
} // namespace openminecraft::renderer::common::demiurge::node

#endif
