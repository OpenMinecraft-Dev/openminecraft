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
    if (stylesStorage.isModified())
    {
        if (rectId == -1)
        {
            rectId = handler->roundedRect.request();
        }

        auto pp = stylesStorage.get<OMDemiurgeRect>("layoutBound");
        auto t = handler->roundedRect.temporary(rectId);
        t->color = genLinear(stylesStorage.get<int>("color"));
        t->position = {pp.x, pp.y, pp.width, pp.height};
        t->depth = depth;
        t->factor = 1.0f;
        t->radius = {0, 0, 0, 0};

        stylesStorage.solve();
    }

    for (auto c : children)
    {
        c->submit(handler, depth - 0.01f);
    }
}
} // namespace openminecraft::renderer::common::demiurge::node
