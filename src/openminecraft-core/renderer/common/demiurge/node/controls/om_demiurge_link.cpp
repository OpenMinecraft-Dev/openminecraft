#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_link.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_label.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

#include <algorithm>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
OMDemiurgeLink::OMDemiurgeLink(geom::OMFontSet *fontset)
    : hoverAmount(0.0f), pressAmount(0.0f), disabledAmount(0.0f), textColor(glm::vec4(0.35f, 0.7f, 1.0f, 1.0f)),
      underlineColor(glm::vec4(0.35f, 0.7f, 1.0f, 1.0f))
{
    setFocusable(true);

    stylesStorage.put("width", OMDemiurgeSize::fit());
    stylesStorage.put("height", OMDemiurgeSize::fit());

    if (fontset != nullptr)
    {
        textNode = std::make_shared<OMDemiurgeLabel>(fontset);
        this->mountDirect(textNode);
    }

    underlineNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"position", OMDemiurgePosition::Absolute},
        {"offsetX", OMDemiurgeSize::pixels(0.0f)},
        {"offsetY", OMDemiurgeSize::pixels(0.0f)},
        {"width", OMDemiurgeSize::fill()},
        {"height", OMDemiurgeSize::pixels(1.0f)},
    });

    this->mountDirect(underlineNode);
}

OMDemiurgeLink::~OMDemiurgeLink() = default;

auto OMDemiurgeLink::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    OMDemiurgePane::submit(handler, depth);
}

void OMDemiurgeLink::update()
{
    const float hover = std::clamp(hoverAmount.get(), 0.0f, 1.0f);
    const float press = std::clamp(pressAmount.get(), 0.0f, 1.0f);
    const float disabledValue = std::clamp(disabledAmount.get(), 0.0f, 1.0f);

    const auto text = textColor.get();
    glm::vec3 textRgb = glm::mix(glm::vec3(text), glm::vec3(1.0f), hover * 0.2f);
    textRgb = glm::mix(textRgb, glm::vec3(0.0f), press * 0.25f);
    textRgb = glm::mix(textRgb, glm::vec3(0.55f), disabledValue * 0.75f);

    if (textNode)
    {
        const int textHeight = stylesStorage.get<int>("textHeight", 14);
        const float textOffset =
            stylesStorage.get<float>("textOffset", -0.21f * static_cast<float>(textHeight));
        textNode->style("text", stylesStorage.get<std::string>("text", "Link"))
            ->style("textHeight", textHeight)
            ->style("offsetY", OMDemiurgeSize::pixels(textOffset))
            ->style("color", (int)f4ToRgba(glm::vec4(textRgb, text.a)));
    }

    const auto underline = underlineColor.get();
    float underlineWidth = 0.0f;

    try
    {
        auto bound = fetchLayoutBound();

        if (bound.height > 0.0f)
        {
            underlineNode->style("offsetY", OMDemiurgeSize::pixels(bound.height - 2.0f));
            underlineWidth = bound.width;
        }
    }
    catch (std::bad_any_cast &)
    {
    }

    const float underlineAmount = std::max(hover, press);
    underlineNode->style("color", (int)f4ToRgba(glm::vec4(glm::vec3(underline), underline.a * underlineAmount)))
        ->style("width", OMDemiurgeSize::pixels(std::max(0.0f, underlineWidth)));

    OMDemiurgePane::update();
}

auto OMDemiurgeLink::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    // Keep hover/underline applied while the link is held down.
    captureMouse();
    hoverAmount.animateTo(1.0f, easeOutCirc<float>, 0.12f);
    pressAmount.animateTo(1.0f, easeOutCirc<float>, 0.08f);
    return Handled;
}

auto OMDemiurgeLink::processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    pressAmount.animateTo(0.0f, easeOutCirc<float>, 0.12f);

    // Re-assert the hover state after the click so the underline does not vanish
    // while the cursor is still over the link.
    const bool inside = containsPoint(x, y);

    hoverAmount.animateTo(inside ? 1.0f : 0.0f, easeOutCirc<float>, 0.12f);

    if (inside)
    {
        handle();
    }

    return Handled;
}

auto OMDemiurgeLink::processMouseEnter(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    hoverAmount.animateTo(1.0f, easeOutCirc<float>, 0.18f);
    return Handled;
}

auto OMDemiurgeLink::processMouseExit(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    hoverAmount.animateTo(0.0f, easeOutCirc<float>, 0.18f);
    return Handled;
}

void OMDemiurgeLink::setText(const std::string &text)
{
    stylesStorage.put("text", text);
}

void OMDemiurgeLink::setTextColor(glm::vec4 color)
{
    textColor.animateTo(color, easeOutCirc<float>, 0.18f);
}

void OMDemiurgeLink::setUnderlineColor(glm::vec4 color)
{
    underlineColor.animateTo(color, easeOutCirc<float>, 0.18f);
}

void OMDemiurgeLink::setOnClick(std::function<void()> handler)
{
    handle = std::move(handler);
}

void OMDemiurgeLink::setDisabled(bool value, bool animate)
{
    if (disabled == value)
    {
        return;
    }

    disabled = value;

    if (animate)
    {
        disabledAmount.animateTo(disabled ? 1.0f : 0.0f, easeOutCirc<float>, 0.18f);
    }
    else
    {
        disabledAmount.setTo(disabled ? 1.0f : 0.0f);
    }
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
