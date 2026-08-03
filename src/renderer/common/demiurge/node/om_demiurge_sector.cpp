
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_sector.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

namespace openminecraft::renderer::common::demiurge::node
{
OMDemiurgeSectorNode::OMDemiurgeSectorNode() = default;
OMDemiurgeSectorNode::~OMDemiurgeSectorNode() = default;

auto OMDemiurgeSectorNode::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    if (sectorId == -1)
    {
        sectorId = handler->sector.request(depth);
        this->handler = handler;
        goto update;
    }
    if (stylesStorage.isModified())
    {
    update:
        auto pp = stylesStorage.get<OMDemiurgeRect>("layoutBound");
        auto t = handler->sector.temporary(sectorId);
        t->color = genLinear(stylesStorage.get<int>("color", 0));
        t->position = {pp.x, pp.y, pp.width, pp.height};
        t->depth = depth;
        t->factor = stylesStorage.get<float>("factor", 2.0f);
        t->radius = stylesStorage.get<float>("radius", 10);
        t->beginAngle = stylesStorage.get<float>("beginAngle", 0.0f);
        t->endAngle = stylesStorage.get<float>("endAngle", 3.1415926 / 2);

        glm::vec3 defaultNormal = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 targetNormal = glm::normalize(stylesStorage.get<glm::vec3>("rotationPivot", {0.0f, 0.0f, 1.0f}));
        auto selfRotation = stylesStorage.get<float>("rotation", 0.0f);

        glm::quat q1 = glm::rotation(defaultNormal, targetNormal);
        glm::quat q2 = glm::angleAxis(selfRotation, targetNormal);

        t->rotation = q2 * q1;

        stylesStorage.solve();
    }

    for (auto c : children)
    {
        c->submit(handler, depth - layerHalfWidth * 2);
    }
}

auto OMDemiurgeSectorNode::remove() -> void
{
    if (sectorId != -1)
    {
        handler->sector.remove(sectorId);
        sectorId = -1;
        handler = nullptr;
    }

    for (auto c : children)
    {
        c->remove();
    }
}
} // namespace openminecraft::renderer::common::demiurge::node
