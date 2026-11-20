#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_tabs.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_label.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

#include <algorithm>
#include <cmath>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::container
{
static auto invalidatePage(const std::shared_ptr<OMDemiurgeNode> &node) -> void;

OMDemiurgeTabPane::OMDemiurgeTabPane(geom::OMFontSet *fontset)
    : disabledAmount(0.0f), indicatorPosition(0.0f), fontset(fontset)
{
    stylesStorage.put("width", OMDemiurgeSize::pixels(360.0f));
    stylesStorage.put("tabHeight", 34.0f);
    stylesStorage.put("tabWidth", 100.0f);
    stylesStorage.put("textHeight", 14);
    stylesStorage.put("activeTextColor", (int)0xffffffff);
    stylesStorage.put("inactiveTextColor", (int)0xffffff8c);
    stylesStorage.put("indicatorColor", (int)0x00d4ffff);
    stylesStorage.put("indicatorHeight", 2.0f);
    stylesStorage.put("barLineColor", (int)0xffffff1a);
    stylesStorage.put("animationSpeed", 0.18f);
    stylesStorage.put("pageAnimationSpeed", 0.25f);
    stylesStorage.put("pageBorder", OMDemiurgeEdgeInsets{2, 2, 2, 2});

    tabBar = std::make_shared<OMDemiurgePane>();
    tabBar->style({{"position", OMDemiurgePosition::Absolute},
                   {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                   {"offsetY", OMDemiurgeSize::pixels(0.0f)},
                   {"width", OMDemiurgeSize::fill()},
                   {"height", OMDemiurgeSize::pixels(34.0f)},
                   {"flexDirection", OMDemiurgeDirection::Row},
                   {"alignItems", OMDemiurgeAlign::Stretch}});
    this->mountDirect(tabBar);

    barLine = std::make_shared<OMDemiurgeRectNode>();
    barLine->style({{"position", OMDemiurgePosition::Absolute},
                    {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                    {"offsetY", OMDemiurgeSize::pixels(33.0f)},
                    {"width", OMDemiurgeSize::fill()},
                    {"height", OMDemiurgeSize::pixels(1.0f)}});
    this->mountDirect(barLine);

    pageHost = std::make_shared<OMDemiurgePane>();
    pageHost->style({{"position", OMDemiurgePosition::Absolute},
                     {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                     {"offsetY", OMDemiurgeSize::pixels(34.0f)},
                     {"width", OMDemiurgeSize::fill()},
                     {"height", OMDemiurgeSize::pixels(0.0f)}});
    this->mountDirect(pageHost);

    pageClip = std::make_shared<OMDemiurgeRectNode>();
    pageClip->style({{"position", OMDemiurgePosition::Absolute},
                     {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                     {"offsetY", OMDemiurgeSize::pixels(0.0f)},
                     {"width", OMDemiurgeSize::fill()},
                     {"height", OMDemiurgeSize::fill()},
                     {"color", (int)0}});
    pageHost->mountDirect(pageClip);

    indicator = std::make_shared<OMDemiurgeRectNode>();
    indicator->style({{"position", OMDemiurgePosition::Absolute},
                      {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                      {"offsetY", OMDemiurgeSize::pixels(0.0f)},
                      {"width", OMDemiurgeSize::pixels(0.0f)},
                      {"height", OMDemiurgeSize::pixels(0.0f)},
                      {"color", (int)0}});
    this->mountDirect(indicator);
}

OMDemiurgeTabPane::~OMDemiurgeTabPane() = default;

auto OMDemiurgeTabPane::addPage(const std::string &title, const std::shared_ptr<OMDemiurgeNode> &page) -> int
{
    if (!page)
    {
        return -1;
    }

    auto root = std::make_shared<OMDemiurgePane>();
    root->style({{"flexGrow", 1.0f},
                 {"height", OMDemiurgeSize::fill()},
                 {"flexDirection", OMDemiurgeDirection::Column},
                 {"justifyContent", OMDemiurgeAlign::Center},
                 {"alignItems", OMDemiurgeAlign::Center},
                 {"padding", OMDemiurgeEdgeInsets{0.0f, 0.0f, 10.0f, 10.0f}}});

    auto label = std::make_shared<controls::OMDemiurgeLabel>(fontset);
    label->style({{"text", title},
                  {"textHeight", 14},
                  {"color", (int)0x8cffffff},
                  {"alignSelf", OMDemiurgeAlign::Center}});
    root->mountDirect(label);

    tabBar->mountDirect(root);

    page->style("border", stylesStorage.get<OMDemiurgeEdgeInsets>("pageBorder", OMDemiurgeEdgeInsets{2, 2, 2, 2}));
    page->style({{"position", OMDemiurgePosition::Absolute},
                 {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                 {"offsetY", OMDemiurgeSize::pixels(0.0f)},
                 {"width", OMDemiurgeSize::fill()},
                 {"height", OMDemiurgeSize::fit()}});
    page->setVisible(false);
    pageHost->mountDirect(page);

    tabs.push_back({root, label, page, title, animation::easeOutCubic<float>});

    if (activeIndex < 0)
    {
        activeIndex = 0;
        page->setVisible(true);
        invalidatePage(page);
        indicatorPosition.setTo(0.0f);
        lastHeight = -1.0f;
    }

    return static_cast<int>(tabs.size()) - 1;
}

auto OMDemiurgeTabPane::setPageVisible(int index, bool visible) -> void
{
    if (index < 0 || index >= static_cast<int>(tabs.size()))
    {
        return;
    }

    tabs[index].page->setVisible(visible);
}

static auto invalidatePage(const std::shared_ptr<OMDemiurgeNode> &node) -> void
{
    node->invalidate();

    for (auto &child : node->fetchChildren())
    {
        invalidatePage(child);
    }
}

auto OMDemiurgeTabPane::applyActivePage() -> void
{
    for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
    {
        setPageVisible(i, i == activeIndex);
    }
}

void OMDemiurgeTabPane::setActivePage(int index)
{
    if (index < 0 || index >= static_cast<int>(tabs.size()) || index == activeIndex)
    {
        return;
    }

    // A new switch during an existing transition snaps the previous one to its end.
    finishPageTransition();

    const int oldIndex = activeIndex;
    auto oldPage = activeIndex >= 0 ? tabs[activeIndex].page : nullptr;

    if (oldPage)
    {
        OMDemiurgeNode::releaseFocusWithin(oldPage);
        oldPage->setVisible(false);
    }

    activeIndex = index;
    tabs[activeIndex].page->setVisible(true);
    invalidatePage(tabs[activeIndex].page);
    indicatorPosition.animateTo(static_cast<float>(activeIndex), easeOutCubic<float>,
                                stylesStorage.get<float>("animationSpeed", 0.22f));
    lastHeight = -1.0f;

    const float speed = stylesStorage.get<float>("pageAnimationSpeed", pageAnimationSpeed);

    if (oldPage && speed > 0.001f)
    {
        // Keep the old page mounted and slide it out; the new page slides in from
        // the direction implied by the tab order (horizontal for top/bottom,
        // vertical for left/right).
        outgoingPage = oldPage;
        outgoingIndex = oldIndex;
        pageTransitioning = true;
        pageTransitionDir = index > oldIndex ? 1.0f : -1.0f;
        pageProgress.setTo(0.0f);
        pageProgress.animateTo(1.0f, animation::linear<float>, speed);
    }
    else if (oldPage)
    {
        oldPage->remove();
    }

    handlePageChange(activeIndex);
}

auto OMDemiurgeTabPane::setPageOffset(const std::shared_ptr<OMDemiurgeNode> &page, float x, float y) -> void
{
    if (!page)
    {
        return;
    }

    page->style("offsetX", OMDemiurgeSize::pixels(x))->style("offsetY", OMDemiurgeSize::pixels(y));
}

auto OMDemiurgeTabPane::finishPageTransition() -> void
{
    if (outgoingPage)
    {
        setPageOffset(outgoingPage, 0.0f, 0.0f);
        outgoingPage->remove();
        outgoingPage->setVisible(false);
        outgoingPage.reset();
    }

    outgoingIndex = -1;
    pageTransitioning = false;

    if (activeIndex >= 0 && activeIndex < static_cast<int>(tabs.size()))
    {
        setPageOffset(tabs[activeIndex].page, 0.0f, 0.0f);
    }
}

auto OMDemiurgeTabPane::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    // Only the active page is submitted; inactive pages stay mounted for layout and
    // state but have no elements in the channels.
    tabBar->submit(handler, tabBar->isLayerOverlay() ? overlayLayer(depth + 1) : depth + 1);
    barLine->submit(handler, barLine->isLayerOverlay() ? overlayLayer(depth + 1) : depth + 1);
    indicator->submit(handler, indicator->isLayerOverlay() ? overlayLayer(depth + 1) : depth + 1);

    if (pageTransitioning && outgoingPage)
    {
        outgoingPage->setClipNode(pageClip);
        outgoingPage->submit(handler, outgoingPage->isLayerOverlay() ? overlayLayer(depth + 1) : depth + 1);
    }

    if (activeIndex >= 0 && activeIndex < static_cast<int>(tabs.size()))
    {
        auto &page = tabs[activeIndex].page;
        page->setClipNode(pageClip);
        page->submit(handler, page->isLayerOverlay() ? overlayLayer(depth + 1) : depth + 1);
    }
}

void OMDemiurgeTabPane::update()
{
    const float tabThicknessStyle = std::max(8.0f, stylesStorage.get<float>("tabHeight", 34.0f));
    const float indicatorThickness = std::max(1.0f, stylesStorage.get<float>("indicatorHeight", 2.0f));
    const float disabledValue = std::clamp(disabledAmount.get(), 0.0f, 1.0f);
    const int textHeight = stylesStorage.get<int>("textHeight", 14);
    const int count = static_cast<int>(tabs.size());
    const bool horizontal = tabPosition == OMDemiurgeTabPosition::Top || tabPosition == OMDemiurgeTabPosition::Bottom;

    // Vertical tabs need a wider bar so labels fit.
    const float tabThickness = horizontal ? tabThicknessStyle : std::max(tabThicknessStyle, 64.0f);
    const float tabWidth = std::max(24.0f, stylesStorage.get<float>("tabWidth", 100.0f));

    float pageHeight = 0.0f;

    if (activeIndex >= 0 && activeIndex < count)
    {
        try
        {
            pageHeight = tabs[activeIndex].page->fetchLayoutBound().height;
        }
        catch (std::bad_any_cast &)
        {
        }
    }

    pageHeight = std::max(0.0f, pageHeight);

    float controlWidth = 360.0f;
    float controlHeight = 0.0f;

    try
    {
        auto bound = this->fetchLayoutBound();
        controlWidth = std::max(1.0f, bound.width);
        controlHeight = bound.height;
    }
    catch (std::bad_any_cast &)
    {
    }

    const float verticalBarHeight =
        horizontal ? tabThickness : tabThicknessStyle * static_cast<float>(std::max(1, count));

    if (pageFill)
    {
        // The page occupies the remaining control area; fall back to content sizing
        // before the first layout pass.
        if (controlHeight <= 1.0f)
        {
            controlHeight = (horizontal ? tabThickness + pageHeight : std::max(pageHeight, verticalBarHeight));
        }

        pageHeight = std::max(0.0f, horizontal ? controlHeight - tabThickness : controlHeight);

        if (activeIndex >= 0 && activeIndex < count)
        {
            tabs[activeIndex].page->style("height", OMDemiurgeSize::fill());
        }
    }
    else
    {
        const float targetHeight = horizontal ? tabThickness + pageHeight : std::max(pageHeight, verticalBarHeight);

        if (std::abs(targetHeight - lastHeight) > 0.01f)
        {
            lastHeight = targetHeight;
            this->style("height", OMDemiurgeSize::pixels(targetHeight));
        }

        if (activeIndex >= 0 && activeIndex < count)
        {
            tabs[activeIndex].page->style("height", OMDemiurgeSize::fit());
        }
    }

    const float targetHeight = pageFill ? controlHeight : (horizontal ? tabThickness + pageHeight : pageHeight);

    const float barLength = horizontal ? tabWidth * static_cast<float>(std::max(1, count))
                                       : tabThicknessStyle * static_cast<float>(std::max(1, count));
    float tabBarX = 0.0f, tabBarY = 0.0f, tabBarW = controlWidth, tabBarH = tabThickness;
    float pageX = 0.0f, pageY = 0.0f, pageW = controlWidth, pageH = pageHeight;

    switch (tabPosition)
    {
    case OMDemiurgeTabPosition::Top:
        tabBarW = barLength;
        tabBarX = 0.0f;
        tabBarY = 0.0f;
        pageY = tabThickness;
        break;
    case OMDemiurgeTabPosition::Bottom:
        tabBarW = barLength;
        tabBarX = 0.0f;
        tabBarY = pageHeight;
        pageY = 0.0f;
        break;
    case OMDemiurgeTabPosition::Left:
        tabBarW = tabThickness;
        tabBarH = verticalBarHeight;
        tabBarX = 0.0f;
        tabBarY = 0.0f;
        pageX = tabThickness;
        pageW = std::max(1.0f, controlWidth - tabThickness);
        break;
    case OMDemiurgeTabPosition::Right:
        tabBarX = std::max(0.0f, controlWidth - tabThickness);
        tabBarW = tabThickness;
        tabBarH = verticalBarHeight;
        tabBarY = 0.0f;
        pageX = 0.0f;
        pageW = std::max(1.0f, controlWidth - tabThickness);
        break;
    }

    // Align the bar along its own axis (horizontal: left/center/right, vertical:
    // top/center/bottom).
    if (horizontal)
    {
        const float free = std::max(0.0f, controlWidth - tabBarW);
        tabBarX = tabAlignment == OMDemiurgeTabAlignment::Start
                      ? 0.0f
                      : (tabAlignment == OMDemiurgeTabAlignment::Center ? free * 0.5f : free);
    }
    else
    {
        const float free = std::max(0.0f, targetHeight - tabBarH);
        tabBarY = tabAlignment == OMDemiurgeTabAlignment::Start
                      ? 0.0f
                      : (tabAlignment == OMDemiurgeTabAlignment::Center ? free * 0.5f : free);
    }

    tabBar->style("offsetX", OMDemiurgeSize::pixels(tabBarX))
        ->style("offsetY", OMDemiurgeSize::pixels(tabBarY))
        ->style("width", OMDemiurgeSize::pixels(tabBarW))
        ->style("height", OMDemiurgeSize::pixels(tabBarH))
        ->style("flexDirection", horizontal ? OMDemiurgeDirection::Row : OMDemiurgeDirection::Column);

    float lineX = 0.0f, lineY = 0.0f, lineW = 0.0f, lineH = 0.0f;

    switch (tabPosition)
    {
    case OMDemiurgeTabPosition::Top:
        lineX = tabBarX;
        lineY = tabBarH - 1.0f;
        lineW = tabBarW;
        lineH = 1.0f;
        break;
    case OMDemiurgeTabPosition::Bottom:
        lineX = tabBarX;
        lineY = pageH;
        lineW = tabBarW;
        lineH = 1.0f;
        break;
    case OMDemiurgeTabPosition::Left:
        lineX = tabBarW - 1.0f;
        lineY = tabBarY;
        lineW = 1.0f;
        lineH = tabBarH;
        break;
    case OMDemiurgeTabPosition::Right:
        lineX = tabBarX;
        lineY = tabBarY;
        lineW = 1.0f;
        lineH = tabBarH;
        break;
    }

    barLine->style("offsetX", OMDemiurgeSize::pixels(lineX))
        ->style("offsetY", OMDemiurgeSize::pixels(lineY))
        ->style("width", OMDemiurgeSize::pixels(lineW))
        ->style("height", OMDemiurgeSize::pixels(lineH))
        ->style("color", (int)stylesStorage.get<int>("barLineColor", (int)0xffffff1a));

    pageHost->style("offsetX", OMDemiurgeSize::pixels(pageX))
        ->style("offsetY", OMDemiurgeSize::pixels(pageY))
        ->style("width", OMDemiurgeSize::pixels(pageW))
        ->style("height", OMDemiurgeSize::pixels(pageH));

    auto dim = [&](uint32_t rgba) -> uint32_t {
        const float r = static_cast<float>((rgba >> 24) & 0xff) / 255.0f;
        const float g = static_cast<float>((rgba >> 16) & 0xff) / 255.0f;
        const float b = static_cast<float>((rgba >> 8) & 0xff) / 255.0f;
        const float a = static_cast<float>(rgba & 0xff) / 255.0f;
        const glm::vec3 rgb = glm::mix(glm::vec3(r, g, b), glm::vec3(0.3f), disabledValue * 0.6f);
        return static_cast<uint32_t>(f4ToRgba(glm::vec4(rgb, a * (1.0f - disabledValue * 0.35f))));
    };

    const uint32_t activeColor = static_cast<uint32_t>(stylesStorage.get<int>("activeTextColor", (int)0xffffffff));
    const uint32_t inactiveColor =
        static_cast<uint32_t>(stylesStorage.get<int>("inactiveTextColor", (int)0xffffff8c));
    const uint32_t indicatorColor = static_cast<uint32_t>(stylesStorage.get<int>("indicatorColor", (int)0x00d4ffff));

    const float indicatorSide = indicatorThickness + 4.0f;

    for (int i = 0; i < count; ++i)
    {
        auto &tab = tabs[i];
        const bool active = i == activeIndex;
        const auto padding =
            horizontal ? OMDemiurgeEdgeInsets{0.0f, 0.0f, 10.0f, 10.0f}
                       : (tabPosition == OMDemiurgeTabPosition::Left
                              ? OMDemiurgeEdgeInsets{0.0f, 0.0f, 2.0f, indicatorSide}
                              : OMDemiurgeEdgeInsets{0.0f, 0.0f, indicatorSide, 2.0f});

        tab.root->style("padding", padding)
            ->style("flexGrow", 0.0f)
            ->style("width", horizontal ? OMDemiurgeSize::pixels(tabWidth) : OMDemiurgeSize::fill())
            ->style("height", horizontal ? OMDemiurgeSize::fill() : OMDemiurgeSize::pixels(tabThicknessStyle));
        tab.label->style("textHeight", textHeight)
            ->style("color", (int)dim(active ? activeColor : inactiveColor));
    }

    // Single sliding indicator.
    const float position = std::clamp(indicatorPosition.get(), 0.0f, static_cast<float>(std::max(0, count - 1)));
    float indX = 0.0f, indY = 0.0f, indW = 0.0f, indH = 0.0f;

    if (count > 0)
    {
        if (horizontal)
        {
            const float extent = tabWidth;
            indX = tabBarX + extent * position;
            indY = (tabPosition == OMDemiurgeTabPosition::Top) ? tabBarY + tabBarH - indicatorThickness : tabBarY;
            indW = extent;
            indH = indicatorThickness;
        }
        else
        {
            const float extent = tabThicknessStyle;
            indX = (tabPosition == OMDemiurgeTabPosition::Left) ? tabBarX + tabBarW - indicatorThickness : tabBarX;
            indY = tabBarY + extent * position;
            indW = indicatorThickness;
            indH = extent;
        }
    }

    // Keep the indicator strictly within the mounted tab range. Indicator offsets
    // are local to this control, while tab bounds are absolute, so convert first.
    if (count > 0)
    {
        try
        {
            auto self = this->fetchLayoutBound();

            if (horizontal)
            {
                auto first = tabs.front().root->fetchLayoutBound();
                auto last = tabs.back().root->fetchLayoutBound();
                const float minX = first.x - self.x;
                const float maxX = std::max(minX, last.x + last.width - indW - self.x);
                indX = std::clamp(indX, minX, maxX);
            }
            else
            {
                auto first = tabs.front().root->fetchLayoutBound();
                auto last = tabs.back().root->fetchLayoutBound();
                const float minY = first.y - self.y;
                const float maxY = std::max(minY, last.y + last.height - indH - self.y);
                indY = std::clamp(indY, minY, maxY);
            }
        }
        catch (std::bad_any_cast &)
        {
        }
    }

    indicator->style("offsetX", OMDemiurgeSize::pixels(indX))
        ->style("offsetY", OMDemiurgeSize::pixels(indY))
        ->style("width", OMDemiurgeSize::pixels(indW))
        ->style("height", OMDemiurgeSize::pixels(indH))
        ->style("color", (int)(count > 0 ? dim(indicatorColor) : 0));

    if (activeIndex >= 0 && activeIndex < count)
    {
        auto &activePage = tabs[activeIndex].page;

        if (pageTransitioning)
        {
            const float progress = std::clamp(pageProgress.get(), 0.0f, 1.0f);

            if (progress >= 0.999f)
            {
                finishPageTransition();
            }
            else
            {
                const auto &easing = tabs[activeIndex].easing;
                const float eased = easing ? easing(progress) : progress;
                const float extent = horizontal ? pageW : pageH;

                if (horizontal)
                {
                    setPageOffset(activePage, pageTransitionDir * extent * (1.0f - eased), 0.0f);
                    setPageOffset(outgoingPage, -pageTransitionDir * extent * eased, 0.0f);
                }
                else
                {
                    setPageOffset(activePage, 0.0f, pageTransitionDir * extent * (1.0f - eased));
                    setPageOffset(outgoingPage, 0.0f, -pageTransitionDir * extent * eased);
                }
            }
        }
        else
        {
            setPageOffset(activePage, 0.0f, 0.0f);
        }
    }

    tabBar->update();
    barLine->update();
    indicator->update();

    if (activeIndex >= 0 && activeIndex < count)
    {
        tabs[activeIndex].page->setClipNode(pageClip);
        tabs[activeIndex].page->update();
    }
}

auto OMDemiurgeTabPane::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled || button != 1)
    {
        return Ignored;
    }

    for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
    {
        if (tabs[i].root->containsPoint(x, y))
        {
            setActivePage(i);
            return Handled;
        }
    }

    return Ignored;
}

void OMDemiurgeTabPane::setTabWidth(float pixels)
{
    stylesStorage.put("tabWidth", std::max(24.0f, pixels));
    lastHeight = -1.0f;
}

void OMDemiurgeTabPane::setPageFill(bool value)
{
    if (pageFill == value)
    {
        return;
    }

    pageFill = value;
    lastHeight = -1.0f;
}

void OMDemiurgeTabPane::setTabAlignment(OMDemiurgeTabAlignment alignment)
{
    if (tabAlignment == alignment)
    {
        return;
    }

    tabAlignment = alignment;
    lastHeight = -1.0f;
}

void OMDemiurgeTabPane::setTabPosition(OMDemiurgeTabPosition position)
{
    if (tabPosition == position)
    {
        return;
    }

    finishPageTransition();
    tabPosition = position;
    lastHeight = -1.0f;
}

void OMDemiurgeTabPane::setPageEasing(int index, animation::OMEasingFunc easing)
{
    if (index < 0 || index >= static_cast<int>(tabs.size()))
    {
        return;
    }

    tabs[index].easing = std::move(easing);
}

void OMDemiurgeTabPane::setPageAnimationSpeed(float seconds)
{
    pageAnimationSpeed = std::max(0.0f, seconds);
    stylesStorage.put("pageAnimationSpeed", pageAnimationSpeed);
}

void OMDemiurgeTabPane::setOnPageChange(std::function<void(int)> handler)
{
    handlePageChange = std::move(handler);
}

void OMDemiurgeTabPane::setDisabled(bool value, bool animate)
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
