#include "openminecraft/renderer/common/demiurge/node/om_demiurge_svg.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

namespace openminecraft::renderer::common::demiurge::node
{
OMDemiurgeSvgNode::OMDemiurgeSvgNode() = default;
OMDemiurgeSvgNode::~OMDemiurgeSvgNode() = default;

auto OMDemiurgeSvgNode::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
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
        t->color.a *= fetchOpacity();
        t->depth = depth;
        t->factor = stylesStorage.get<float>("factor", 0.01f);
        t->scale = stylesStorage.get<glm::vec2>("scale", glm::vec2(1.0f));
        t->glyphId = handler->svg.storeGlyph(stylesStorage.get<std::string>("svgPath", ""),
                                             stylesStorage.get<glm::vec2>("svgSize", {10.0f, 10.0f}));

        glm::vec3 defaultNormal = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 targetNormal = glm::normalize(stylesStorage.get<glm::vec3>("rotationPivot", {0.0f, 0.0f, 1.0f}));
        auto selfRotation = stylesStorage.get<float>("rotation", 0.0f);
        t->rotation = glm::angleAxis(selfRotation, targetNormal) * glm::rotation(defaultNormal, targetNormal);

        stylesStorage.solve();
    }

    handler->applyClip(handler->svg, svgId, resolveClipBound());

    for (auto c : children)
    {
        c->submit(handler, c->isLayerOverlay() ? overlayLayer(depth + 1) : depth + 1);
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