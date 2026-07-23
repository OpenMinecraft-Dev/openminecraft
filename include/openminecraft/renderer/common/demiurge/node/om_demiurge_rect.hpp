#ifndef OM_DEMIURGE_RECT_HPP
#define OM_DEMIURGE_RECT_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
namespace openminecraft::renderer::common::demiurge::node
{
class OMDemiurgeRectNode : public OMDemiurgeNode
{
  public:
    OMDemiurgeRectNode();
    virtual ~OMDemiurgeRectNode();

    auto render(OMRendererTask *, OMDemiurgeRendererHandler *, float depth) -> void override;

  private:
    OMRendererBuffer *vertexBuffer = nullptr;
    OMRendererBuffer *indexBuffer = nullptr;
    OMRendererBuffer *instanceBuffer = nullptr;
};
} // namespace openminecraft::renderer::common::demiurge::node

#endif
