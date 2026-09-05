#include "openminecraft/renderer/common/demiurge/node/om_demiurge_cliprect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

namespace openminecraft::renderer::common::demiurge::node
{
OMDemiurgeClipRectNode::OMDemiurgeClipRectNode() = default;
OMDemiurgeClipRectNode::~OMDemiurgeClipRectNode() = default;

auto OMDemiurgeClipRectNode::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    if (rectId == -1)
    {
        rectId = handler->clipRect.request(depth);
        this->handler = handler;
        goto update;
    }
    if (stylesStorage.isModified())
    {
    update:
        auto pp = stylesStorage.get<OMDemiurgeRect>("layoutBound");
        auto t = handler->clipRect.temporary(rectId);
        t->position = {pp.x, pp.y, pp.width, pp.height};
        t->depth = depth;

        stylesStorage.solve();
    }

    for (auto c : children)
    {
        c->submit(handler, depth - layerHalfWidth * 2);
    }
}

auto OMDemiurgeClipRectNode::remove() -> void
{
    if (rectId != -1)
    {
        handler->clipRect.remove(rectId);
        rectId = -1;
        handler = nullptr;
    }

    for (auto c : children)
    {
        c->remove();
    }
}
} // namespace openminecraft::renderer::common::demiurge::node
