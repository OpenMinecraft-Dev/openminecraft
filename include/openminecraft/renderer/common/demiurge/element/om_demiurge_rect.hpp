#ifndef OM_DEMIURGE_RECT_HPP
#define OM_DEMIURGE_RECT_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
namespace openminecraft::renderer::common::demiurge::element
{
class OMDemiurgeRectElement : public OMDemiurgeNode
{
  public:
    OMDemiurgeRectElement();
    virtual ~OMDemiurgeRectElement();

    auto render(OMRendererTask *, OMDemiurgeRendererHandler *) -> void override;

  private:
    OMRendererBuffer *vertexBuffer = nullptr;
    OMRendererBuffer *indexBuffer = nullptr;
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
