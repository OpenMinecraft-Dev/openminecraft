#ifndef OM_DEMIURGE_SHADERSCENE_HPP
#define OM_DEMIURGE_SHADERSCENE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <string>

namespace openminecraft::renderer::common::demiurge::node
{
class OMDemiurgeShaderSceneNode : public OMDemiurgeNode
{
  public:
    OMDemiurgeShaderSceneNode(std::string shader);
    virtual ~OMDemiurgeShaderSceneNode();

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    auto remove() -> void override;

  private:
    std::string shader;
    int sceneId = -1;
    OMDemiurgeRendererHandler *handler = nullptr;
};
} // namespace openminecraft::renderer::common::demiurge::node

#endif
