#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_dropdown.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_svg.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

#include <algorithm>
#include <cmath>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::container
{
static constexpr const char *ChevronPath = "M 3 6 L 8 11 L 13 6";

static auto dimColor(uint32_t rgba, float amount) -> glm::vec4
{
    const glm::vec4 color = genLinear(rgba);
    const glm::vec3 rgb =
        glm::mix(glm::vec3(color), glm::vec3(genLinear(0x303030ff)), std::clamp(amount, 0.0f, 1.0f) * 0.6f);
    return glm::vec4(rgb, color.a * (1.0f - std::clamp(amount, 0.0f, 1.0f) * 0.35f));
}

OMDemiurgeDropDownPane::OMDemiurgeDropDownPane(geom::OMFontSet *fontset)
    : openAmount(0.0f), disabledAmount(0.0f), fontset(fontset)
{
    setFocusable(true);

    stylesStorage.put("width", OMDemiurgeSize::pixels(280.0f));
    stylesStorage.put("radius", glm::vec4(8.0f));
    stylesStorage.put("color", (int)f4ToRgba(glm::vec4(0.13f, 0.13f, 0.16f, 0.55f)));
    stylesStorage.put("textColor", (int)0xffffffff);
    stylesStorage.put("arrowColor", (int)0xe6ffffff);
    stylesStorage.put("textHeight", 14);
    stylesStorage.put("label", "Details");
    stylesStorage.put("headerHeight", 34.0f);
    stylesStorage.put("animationSpeed", 0.22f);

    background = std::make_shared<OMDemiurgeRectNode>();
    background->style({{"position", OMDemiurgePosition::Absolute},
                       {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                       {"offsetY", OMDemiurgeSize::pixels(0.0f)},
                       {"width", OMDemiurgeSize::fill()},
                       {"height", OMDemiurgeSize::fill()}});
    this->mountDirect(background);

    header = std::make_shared<OMDemiurgePane>();
    header->style({{"position", OMDemiurgePosition::Absolute},
                   {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                   {"offsetY", OMDemiurgeSize::pixels(0.0f)},
                   {"width", OMDemiurgeSize::fill()},
                   {"height", OMDemiurgeSize::pixels(headerHeight)},
                   {"flexDirection", OMDemiurgeDirection::Row},
                   {"alignItems", OMDemiurgeAlign::Center},
                   {"padding", OMDemiurgeEdgeInsets{0.0f, 0.0f, 12.0f, 10.0f}}});

    titleNode = std::make_shared<OMDemiurgeTextSdfNode>(fontset);
    titleNode->style({{"text", "Details"},
                      {"textHeight", 14},
                      {"color", (int)0xffffffff},
                      {"flexGrow", 1.0f},
                      {"alignSelf", OMDemiurgeAlign::Center}});
    header->mountDirect(titleNode);

    arrowNode = std::make_shared<OMDemiurgeSvgNode>();
    arrowNode->style({{"width", OMDemiurgeSize::pixels(16.0f)},
                      {"height", OMDemiurgeSize::pixels(16.0f)},
                      {"svgPath", std::string(ChevronPath)},
                      {"svgSize", glm::vec2(16.0f, 16.0f)},
                      {"factor", 0.06f},
                      {"color", (int)0xe6ffffff},
                      {"alignSelf", OMDemiurgeAlign::Center},
                      {"rotation", 0.0f}});
    header->mountDirect(arrowNode);

    this->mountDirect(header);

    contentHost = std::make_shared<OMDemiurgePane>();
    contentHost->style({{"position", OMDemiurgePosition::Absolute},
                        {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                        {"offsetY", OMDemiurgeSize::pixels(headerHeight)},
                        {"width", OMDemiurgeSize::fill()},
                        {"height", OMDemiurgeSize::fit()},
                        {"flexDirection", OMDemiurgeDirection::Column}});
    this->mountDirect(contentHost);
}

OMDemiurgeDropDownPane::~OMDemiurgeDropDownPane() = default;

auto OMDemiurgeDropDownPane::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    // A parent pane assigns its own clip to the whole subtree; keep the expanding
    // content clipped to the dropdown body as well.
    contentHost->setClipNode(std::static_pointer_cast<OMDemiurgeDropDownPane>(shared_from_this()));
    OMDemiurgePane::submit(handler, depth);
}

void OMDemiurgeDropDownPane::update()
{
    const float disabledValue = std::clamp(disabledAmount.get(), 0.0f, 1.0f);
    const float open = std::clamp(openAmount.get(), 0.0f, 1.0f);
    const float headerH = std::max(8.0f, stylesStorage.get<float>("headerHeight", headerHeight));
    headerHeight = headerH;

    // Natural content height comes from the absolute content host, which is
    // independent of the animated dropdown height.
    float measuredContentHeight = 0.0f;

    try
    {
        measuredContentHeight = contentHost->fetchLayoutBound().height;
    }
    catch (std::bad_any_cast &)
    {
    }

    if (measuredContentHeight > 0.5f)
    {
        contentHeight = measuredContentHeight;
    }

    const float targetHeight = headerH + contentHeight * open;

    if (std::abs(targetHeight - lastHeight) > 0.01f)
    {
        lastHeight = targetHeight;
        this->style("height", OMDemiurgeSize::pixels(targetHeight));
    }

    const auto radius = stylesStorage.get<glm::vec4>("radius", glm::vec4(8.0f));
    const auto bg = dimColor(static_cast<uint32_t>(stylesStorage.get<int>("color", (int)0x2222298c)), disabledValue);
    const auto text =
        dimColor(static_cast<uint32_t>(stylesStorage.get<int>("textColor", (int)0xffffffff)), disabledValue);
    const auto arrow =
        dimColor(static_cast<uint32_t>(stylesStorage.get<int>("arrowColor", (int)0xe6ffffff)), disabledValue);

    background->style("color", (int)f4ToRgba(bg))->style("radius", radius);

    header->style("height", OMDemiurgeSize::pixels(headerH))
        ->style("padding",
                stylesStorage.get<OMDemiurgeEdgeInsets>("headerPadding", OMDemiurgeEdgeInsets{0, 0, 12, 10}));

    const int textHeight = stylesStorage.get<int>("textHeight", 14);
    titleNode->style("text", stylesStorage.get<std::string>("label", "Details"))
        ->style("textHeight", textHeight)
        ->style("color", (int)f4ToRgba(text));

    arrowNode->style("color", (int)f4ToRgba(arrow))
        ->style("svgPath", stylesStorage.get<std::string>("arrowPath", std::string(ChevronPath)))
        ->style("rotation", open * 3.1415926535f);

    contentHost->style("offsetY", OMDemiurgeSize::pixels(headerH));
    contentHost->setClipNode(std::static_pointer_cast<OMDemiurgeDropDownPane>(shared_from_this()));

    OMDemiurgePane::update();
}

auto OMDemiurgeDropDownPane::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled || button != 1)
    {
        return Ignored;
    }

    float localY = y;

    try
    {
        localY = y - this->fetchLayoutBound().y;
    }
    catch (std::bad_any_cast &)
    {
    }

    if (localY < 0.0f || localY > headerHeight)
    {
        return Ignored;
    }

    pressed = true;
    captureMouse();
    return Handled;
}

auto OMDemiurgeDropDownPane::processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (!pressed || button != 1)
    {
        return Ignored;
    }

    pressed = false;

    float localY = y;

    try
    {
        localY = y - this->fetchLayoutBound().y;
    }
    catch (std::bad_any_cast &)
    {
    }

    if (localY >= 0.0f && localY <= headerHeight)
    {
        toggle(true);
    }

    return Handled;
}

void OMDemiurgeDropDownPane::setTitle(const std::string &title)
{
    stylesStorage.put("label", title);
}

void OMDemiurgeDropDownPane::setContentNode(const std::shared_ptr<OMDemiurgeNode> &node)
{
    if (content)
    {
        contentHost->umount(content);
        content.reset();
    }

    content = node;

    if (content)
    {
        contentHost->mount(content);
    }

    contentHeight = 0.0f;
}

void OMDemiurgeDropDownPane::setExpanded(bool value, bool animate)
{
    if (expanded == value)
    {
        return;
    }

    expanded = value;
    const float speed = stylesStorage.get<float>("animationSpeed", 0.22f);

    if (animate)
    {
        openAmount.animateTo(expanded ? 1.0f : 0.0f, easeOutCubic<float>, speed);
    }
    else
    {
        openAmount.setTo(expanded ? 1.0f : 0.0f);
    }

    handleToggle(expanded);
}

void OMDemiurgeDropDownPane::toggle(bool animate)
{
    setExpanded(!expanded, animate);
}

void OMDemiurgeDropDownPane::setOnToggle(std::function<void(bool)> handler)
{
    handleToggle = std::move(handler);
}

void OMDemiurgeDropDownPane::setHeaderHeight(float pixels)
{
    headerHeight = std::max(8.0f, pixels);
    stylesStorage.put("headerHeight", headerHeight);
}

void OMDemiurgeDropDownPane::setAnimationSpeed(float seconds)
{
    stylesStorage.put("animationSpeed", std::max(0.0f, seconds));
}

void OMDemiurgeDropDownPane::setDisabled(bool value, bool animate)
{
    if (disabled == value)
    {
        return;
    }

    disabled = value;

    if (animate)
    {
        disabledAmount.animateTo(value ? 1.0f : 0.0f, easeOutCirc<float>,
                                 stylesStorage.get<float>("animationSpeed", 0.18f));
    }
    else
    {
        disabledAmount.setTo(value ? 1.0f : 0.0f);
    }
}
} // namespace openminecraft::renderer::common::demiurge::node::container
