#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"

namespace openminecraft::renderer::common::demiurge::node
{
OMDemiurgeRectNode::OMDemiurgeRectNode() = default;
OMDemiurgeRectNode::~OMDemiurgeRectNode() = default;

auto OMDemiurgeRectNode::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    if (rectId == -1)
    {
        rectId = handler->roundedRect.request(depth);
        this->handler = handler;
        goto update;
    }
    if (stylesStorage.isModified())
    {
    update:
        auto pp = stylesStorage.get<OMDemiurgeRect>("layoutBound");
        auto t = handler->roundedRect.temporary(rectId);
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

auto OMDemiurgeRectNode::remove() -> void
{
    if (rectId != -1)
    {
        handler->roundedRect.remove(rectId);
        rectId = -1;
        handler = nullptr;
    }

    for (auto c : children)
    {
        c->remove();
    }
}
} // namespace openminecraft::renderer::common::demiurge::node
