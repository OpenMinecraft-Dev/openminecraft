#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_multiline_label.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
OMDemiurgeMultilineLabel::OMDemiurgeMultilineLabel(geom::OMFontSet *fontset)
    : OMDemiurgeTextSdfMultilineNode(fontset)
{
}

void OMDemiurgeMultilineLabel::update()
{
    if (colorAnimated)
    {
        const auto current = labelColor.get();

        if (current != lastColor)
        {
            this->style("color", static_cast<int>(f4ToRgba(current)));
            lastColor = current;
        }
    }

    OMDemiurgeTextSdfMultilineNode::update();
}

void OMDemiurgeMultilineLabel::setText(const std::string &value)
{
    this->style("text", value);
}

auto OMDemiurgeMultilineLabel::getText() -> std::string
{
    return stylesStorage.get<std::string>("text", "");
}

void OMDemiurgeMultilineLabel::setColor(const glm::vec4 &value)
{
    const auto current = getColor();
    labelColor.setTo(current);
    labelColor.animateTo(value, easeOutCirc<float>, stylesStorage.get<float>("animationSpeed", 0.18f));
    lastColor = current;
    colorAnimated = true;
}

auto OMDemiurgeMultilineLabel::getColor() -> glm::vec4
{
    if (colorAnimated)
    {
        return labelColor.get();
    }

    return genLinear(static_cast<uint32_t>(stylesStorage.get<int>("color", 0)));
}

void OMDemiurgeMultilineLabel::setAnimationSpeed(float seconds)
{
    stylesStorage.put("animationSpeed", seconds);
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
