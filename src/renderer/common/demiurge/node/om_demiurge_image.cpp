#include "openminecraft/renderer/common/demiurge/node/om_demiurge_image.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"

namespace openminecraft::renderer::common::demiurge::node
{
OMDemiurgeImageNode::OMDemiurgeImageNode(OMRendererTexture *texture)
{
    this->texture = texture;
}

OMDemiurgeImageNode::~OMDemiurgeImageNode() = default;

auto OMDemiurgeImageNode::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    if (imageId == -1)
    {
        imageId = handler->image.request(depth);
        this->handler = handler;
        handler->image.textures[imageId] = texture;
        goto update;
    }
    if (stylesStorage.isModified())
    {
    update:
        auto pp = stylesStorage.get<OMDemiurgeRect>("layoutBound");
        auto t = handler->image.temporary(imageId);
        t->color = genLinear(stylesStorage.get<int>("color", 0));
        t->position = {pp.x, pp.y, pp.width, pp.height};
        t->depth = depth;
        t->factor = stylesStorage.get<float>("factor", 2.0f);
        t->radius = stylesStorage.get<glm::vec4>("radius", {0, 0, 0, 0});

        stylesStorage.solve();
    }

    for (auto c : children)
    {
        c->submit(handler, depth - layerHalfWidth * 2);
    }
}
auto OMDemiurgeImageNode::remove() -> void
{
    if (imageId != -1)
    {
        handler->image.remove(imageId);
        handler->image.textures.erase(imageId);

        imageId = -1;
        handler = nullptr;
    }

    for (auto c : children)
    {
        c->remove();
    }
}
} // namespace openminecraft::renderer::common::demiurge::node
