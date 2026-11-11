#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_divider.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
OMDemiurgeDivider::OMDemiurgeDivider(bool vertical)
    : lineColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.15f)), vertical(vertical)
{
    if (vertical)
    {
        stylesStorage.put("width", OMDemiurgeSize::pixels(thickness));
        stylesStorage.put("height", OMDemiurgeSize::fill());
    }
    else
    {
        stylesStorage.put("width", OMDemiurgeSize::fill());
        stylesStorage.put("height", OMDemiurgeSize::pixels(thickness));
    }

    lineNode = std::make_shared<OMDemiurgeRectNode>();
    this->mountDirect(lineNode);
}

OMDemiurgeDivider::~OMDemiurgeDivider() = default;

auto OMDemiurgeDivider::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    OMDemiurgeContainerNode::submit(handler, depth);
}

void OMDemiurgeDivider::update()
{
    lineNode->style("color", (int)f4ToRgba(lineColor.get()))
        ->style("width", vertical ? OMDemiurgeSize::pixels(thickness) : OMDemiurgeSize::fill())
        ->style("height", vertical ? OMDemiurgeSize::fill() : OMDemiurgeSize::pixels(thickness));

    OMDemiurgeContainerNode::update();
}

void OMDemiurgeDivider::setColor(glm::vec4 color)
{
    lineColor.animateTo(color, easeOutCirc<float>, 0.18f);
}

void OMDemiurgeDivider::setThickness(float pixels)
{
    thickness = pixels;
    stylesStorage.put(vertical ? "width" : "height", OMDemiurgeSize::pixels(pixels));
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
