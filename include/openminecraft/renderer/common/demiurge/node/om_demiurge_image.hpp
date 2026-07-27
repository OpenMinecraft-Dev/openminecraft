#ifndef OM_DEMIURGE_IMAGE_HPP
#define OM_DEMIURGE_IMAGE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
namespace openminecraft::renderer::common::demiurge::node
{
class OMDemiurgeImageNode : public OMDemiurgeNode
{
  public:
    OMDemiurgeImageNode(OMRendererTexture *texture);
    virtual ~OMDemiurgeImageNode();

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    auto remove() -> void override;

  private:
    int imageId = -1;
    OMRendererTexture *texture = nullptr;
    OMDemiurgeRendererHandler *handler = nullptr;
};
} // namespace openminecraft::renderer::common::demiurge::node

#endif
