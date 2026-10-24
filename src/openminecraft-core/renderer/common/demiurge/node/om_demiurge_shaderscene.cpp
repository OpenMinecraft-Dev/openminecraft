#include "openminecraft/renderer/common/demiurge/node/om_demiurge_shaderscene.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"

namespace openminecraft::renderer::common::demiurge::node
{
OMDemiurgeShaderSceneNode::OMDemiurgeShaderSceneNode(std::string shader) : shader(std::move(shader))
{
}

OMDemiurgeShaderSceneNode::~OMDemiurgeShaderSceneNode() = default;

auto OMDemiurgeShaderSceneNode::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    bool created = false;

    if (sceneId == -1)
    {
        sceneId = handler->shaderScene.request(depth);
        this->handler = handler;

        handler->shaderScene.bindShader(sceneId, shader);

        created = true;
    }

    if (created || stylesStorage.isModified())
    {
        auto layoutBound = stylesStorage.get<OMDemiurgeRect>("layoutBound");

        auto *t = handler->shaderScene.temporary(sceneId);
        t->position = {layoutBound.x, layoutBound.y, layoutBound.width, layoutBound.height};
        t->depth = depth;

        stylesStorage.solve();
    }

    for (auto c : children)
    {
        c->submit(handler, depth - layerHalfWidth * 2);
    }
}

auto OMDemiurgeShaderSceneNode::remove() -> void
{
    if (sceneId != -1)
    {
        handler->shaderScene.remove(sceneId);

        sceneId = -1;
        handler = nullptr;
    }

    for (auto c : children)
    {
        c->remove();
    }
}
} // namespace openminecraft::renderer::common::demiurge::node
