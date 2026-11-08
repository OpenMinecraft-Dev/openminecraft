#ifndef OM_DEMIURGE_IMAGE_HPP
#define OM_DEMIURGE_IMAGE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include <string>
namespace openminecraft::renderer::common::demiurge::node
{
enum OMDemiurgeImageFillType
{
    Fill,
    Contain,
    Cover
};
class OMDemiurgeImageNode : public OMDemiurgeNode
{
  public:
    OMDemiurgeImageNode(std::string textureTag);
    virtual ~OMDemiurgeImageNode();

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    auto remove() -> void override;

  private:
    int imageId = -1;
    std::string textureTag;
    OMDemiurgeRendererHandler *handler = nullptr;
};
} // namespace openminecraft::renderer::common::demiurge::node

#endif
