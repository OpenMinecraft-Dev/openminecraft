#ifndef OM_DEMIURGE_RECT_HPP
#define OM_DEMIURGE_RECT_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
namespace openminecraft::renderer::common::demiurge::node
{
class OMDemiurgeRectNode : public OMDemiurgeNode
{
  public:
    OMDemiurgeRectNode();
    virtual ~OMDemiurgeRectNode();

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    auto remove() -> void override;

  private:
    int rectId = -1;
    OMDemiurgeRendererHandler *handler = nullptr;
};
} // namespace openminecraft::renderer::common::demiurge::node

#endif
