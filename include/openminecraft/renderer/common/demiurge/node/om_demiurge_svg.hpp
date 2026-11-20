#ifndef OM_DEMIURGE_SVG_HPP
#define OM_DEMIURGE_SVG_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
namespace openminecraft::renderer::common::demiurge::node
{
class OMDemiurgeSvgNode : public OMDemiurgeNode
{
  public:
    OMDemiurgeSvgNode();
    virtual ~OMDemiurgeSvgNode();

    auto submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void override;
    auto remove() -> void override;

  private:
    int svgId = -1;
    OMDemiurgeRendererHandler *handler = nullptr;
};
} // namespace openminecraft::renderer::common::demiurge::node

#endif
