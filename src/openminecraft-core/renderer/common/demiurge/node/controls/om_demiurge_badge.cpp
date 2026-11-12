#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_badge.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"

#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"


using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
OMDemiurgeBadge::OMDemiurgeBadge(geom::OMFontSet *fontset)
    : backgroundColor(glm::vec4(0.0f, 0.55f, 0.75f, 1.0f)), textColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
{
    stylesStorage.put("width", OMDemiurgeSize::fit());
    stylesStorage.put("height", OMDemiurgeSize::fit());
    stylesStorage.put("minWidth", 18.0f);
    stylesStorage.put("minHeight", 18.0f);
    stylesStorage.put("radius", glm::vec4(9.0f));
    stylesStorage.put("padding", OMDemiurgeEdgeInsets{2, 2, 8, 8});

    if (fontset != nullptr)
    {
        textNode = std::make_shared<OMDemiurgeTextSdfNode>(fontset);
        this->mountDirect(textNode);
    }
}

OMDemiurgeBadge::~OMDemiurgeBadge() = default;

auto OMDemiurgeBadge::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    OMDemiurgeRectNode::submit(handler, depth);
}

void OMDemiurgeBadge::update()
{
    this->style("internal.color", (int)f4ToRgba(backgroundColor.get()))
        ->style("radius", stylesStorage.get<glm::vec4>("radius", glm::vec4(9.0f)));

    if (textNode)
    {
        textNode->style("text", stylesStorage.get<std::string>("text", "0"))
            ->style("textHeight", stylesStorage.get<int>("textHeight", 12))
            ->style("color", (int)f4ToRgba(textColor.get()));
    }

    OMDemiurgeRectNode::update();
}

void OMDemiurgeBadge::setText(const std::string &text)
{
    stylesStorage.put("text", text);
}

void OMDemiurgeBadge::setBackgroundColor(glm::vec4 color)
{
    backgroundColor.animateTo(color, easeOutCirc<float>, 0.18f);
}

void OMDemiurgeBadge::setTextColor(glm::vec4 color)
{
    textColor.animateTo(color, easeOutCirc<float>, 0.18f);
}

void OMDemiurgeBadge::setTextHeight(int height)
{
    stylesStorage.put("textHeight", height);
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
