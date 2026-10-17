#include "openminecraft/renderer/common/demiurge/node/om_demiurge_svg.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

namespace openminecraft::renderer::common::demiurge::node
{
OMDemiurgeSvgNode::OMDemiurgeSvgNode() = default;
OMDemiurgeSvgNode::~OMDemiurgeSvgNode() = default;

auto OMDemiurgeSvgNode::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    if (svgId == -1)
    {
        svgId = handler->svg.request(depth);
        this->handler = handler;
        goto update;
    }
    if (stylesStorage.isModified())
    {
    update:
        auto layoutBound = stylesStorage.get<OMDemiurgeRect>("layoutBound");
        auto t = handler->svg.temporary(svgId);

        t->position = {layoutBound.x, layoutBound.y, layoutBound.width, layoutBound.height};
        t->color = genLinear(stylesStorage.get<int>("color", 0));
        t->depth = depth;
        t->factor = stylesStorage.get<float>("factor", 0.01f);
        t->glyphId = handler->svg.storeGlyph(stylesStorage.get<std::string>("svgPath", ""),
                                             stylesStorage.get<glm::vec2>("svgSize", {10.0f, 10.0f}));

        stylesStorage.solve();
    }

    for (auto c : children)
    {
        c->submit(handler, depth - layerHalfWidth * 2);
    }
}

auto OMDemiurgeSvgNode::remove() -> void
{
    if (svgId != -1)
    {
        handler->svg.remove(svgId);
        svgId = -1;
        handler = nullptr;
    }

    for (auto c : children)
    {
        c->remove();
    }
}
} // namespace openminecraft::renderer::common::demiurge::node