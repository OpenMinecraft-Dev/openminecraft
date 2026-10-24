#ifndef OM_DEMIURGE_SHADERSCENE_HPP
#define OM_DEMIURGE_SHADERSCENE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <string>

namespace openminecraft::renderer::common::demiurge::node
{
// INFO: 用 fragment shader 填充自身区域的节点。
// shader 需要满足 scene*.frag.glsl 的 uniform 约定：
//   ScreenData { float width; float height; }
//   Time       { float value; }
class OMDemiurgeShaderSceneNode : public OMDemiurgeNode
{
  public:
    // shader 可以是完整相对路径（demiurge/scene/scene1.frag.glsl），也可以只是名字（scene1）
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
