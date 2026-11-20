#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_dropdown_selector.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_pane.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_scrollpane.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_svg.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_label.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"

#include <algorithm>
#include <cmath>
#include <array>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
using container::OMDemiurgeScrollPane;
using container::OMDemiurgeScrollPaneOrientation;
static constexpr const char *ChevronPath = "M 3 6 L 8 11 L 13 6";
static constexpr float ItemHeight = 28.0f;
static constexpr float ItemGap = 2.0f;
static constexpr float PanelPadding = 4.0f;

// Snap the panel height to whole option rows so the list never ends with a
// half-clipped item. Overflowing options remain reachable through the scrollbar.
static auto snapPopupHeight(float height) -> float
{
    const float content = height - PanelPadding * 2.0f;

    if (content < ItemHeight)
    {
        return std::max(44.0f, height);
    }

    const int rows = std::max(1, static_cast<int>(std::floor((content + ItemGap) / (ItemHeight + ItemGap))));
    return PanelPadding * 2.0f + rows * ItemHeight + (rows - 1) * ItemGap;
}

namespace
{
class SelectorItem : public OMDemiurgeButton
{
  public:
    using OMDemiurgeButton::OMDemiurgeButton;

    animation::OMAnimationValue<float> hover{0.0f};
    glm::vec4 normalColor = glm::vec4(0.16f, 0.16f, 0.20f, 0.0f);
    glm::vec4 hoverColor = glm::vec4(0.30f, 0.40f, 0.52f, 0.55f);

    void setItemColors(const glm::vec4 &normal, const glm::vec4 &hovered)
    {
        normalColor = normal;
        hoverColor = hovered;
        backgroundColor.setTo(normal);
        opacity.setTo(1.0f);
    }

    void resetHover()
    {
        hover.setTo(0.0f);
    }

    auto processMouseEnter(float x, float y) -> OMDemiurgeEventResult override
    {
        hover.animateTo(1.0f, animation::easeOutCubic<float>, 0.16f);
        return Handled;
    }

    auto processMouseExit(float x, float y) -> OMDemiurgeEventResult override
    {
        hover.animateTo(0.0f, animation::easeOutCubic<float>, 0.16f);
        return Handled;
    }

    void update() override
    {
        backgroundColor.setTo(glm::mix(normalColor, hoverColor, std::clamp(hover.get(), 0.0f, 1.0f)));
        OMDemiurgeButton::update();
    }
};

class SelectorPopupLayer : public container::OMDemiurgePane
{
  public:
    auto submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void override
    {
        // The selector can be opened above a dialog. Its modal-stack position
        // becomes the overlay z-order; all overlay roots share the real layer.
        int index = -1;

        if (auto root = getRoot())
        {
            index = root->modalIndex(this);
        }

        const uint32_t zorder = static_cast<uint32_t>(std::max(0, index) + 1);
        container::OMDemiurgePane::submit(handler, withLayerZOrder(overlayLayer(0), zorder));
    }
};

class SelectorBackdrop : public OMDemiurgeRectNode
{
  public:
    std::function<void()> onPress;
    std::weak_ptr<OMDemiurgeNode> panel;

    auto processMouseDown(float x, float y, uint8_t) -> OMDemiurgeEventResult override
    {
        if (auto p = panel.lock())
        {
            try
            {
                const auto b = p->fetchLayoutBound();

                if (x >= b.x && x <= b.x + b.width && y >= b.y && y <= b.y + b.height)
                {
                    // The panel and its items handle this click themselves.
                    return Ignored;
                }
            }
            catch (const std::bad_any_cast &)
            {
                // Panel has not been laid out yet; treat the click as outside.
            }
        }

        if (onPress)
        {
            onPress();
        }

        return Handled;
    }
};
} // namespace

OMDemiurgeDropDownSelector::OMDemiurgeDropDownSelector(geom::OMFontSet *fontset) : fontset(fontset)
{
    setFocusable(true);

    stylesStorage.put("width", OMDemiurgeSize::pixels(220.0f));
    stylesStorage.put("height", OMDemiurgeSize::pixels(32.0f));
    stylesStorage.put("flexDirection", OMDemiurgeDirection::Row);
    stylesStorage.put("alignItems", OMDemiurgeAlign::Center);
    stylesStorage.put("padding", OMDemiurgeEdgeInsets{0.0f, 0.0f, 10.0f, 10.0f});

    background = std::make_shared<OMDemiurgeRectNode>();
    background->style({{"position", OMDemiurgePosition::Absolute},
                       {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                       {"offsetY", OMDemiurgeSize::pixels(0.0f)},
                       {"width", OMDemiurgeSize::fill()},
                       {"height", OMDemiurgeSize::fill()},
                       {"radius", glm::vec4(6.0f)},
                       {"color", (int)f4ToRgba(glm::vec4(0.16f, 0.16f, 0.20f, 1.0f))}});
    this->mountDirect(background);

    labelNode = std::make_shared<OMDemiurgeLabel>(fontset);
    labelNode->style({{"text", placeholder},
                      {"textHeight", 13},
                      {"color", (int)0xffffffff},
                      {"flexGrow", 1.0f},
                      {"alignSelf", OMDemiurgeAlign::Center}});
    this->mountDirect(labelNode);

    arrowNode = std::make_shared<OMDemiurgeSvgNode>();
    arrowNode->style({{"width", OMDemiurgeSize::pixels(14.0f)},
                      {"height", OMDemiurgeSize::pixels(14.0f)},
                      {"svgPath", std::string(ChevronPath)},
                      {"svgSize", glm::vec2(16.0f, 16.0f)},
                      {"factor", 0.06f},
                      {"color", (int)0xe6ffffff},
                      {"alignSelf", OMDemiurgeAlign::Center}});
    this->mountDirect(arrowNode);
}

OMDemiurgeDropDownSelector::~OMDemiurgeDropDownSelector()
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    alive->store(false);

    if (scrollPane)
    {
        OMDemiurgeScrollPane::clearOverlayWheelRegion(static_cast<OMDemiurgeScrollPane *>(scrollPane.get()));
    }

    if (popupLayer)
    {
        if (auto parent = popupParent.lock())
        {
            parent->clearModalChild(popupLayer);
            parent->umount(popupLayer);
        }

        popupLayer.reset();
    }
}

auto OMDemiurgeDropDownSelector::processMouseEnter(float, float) -> OMDemiurgeEventResult
{
    if (disabled.load())
    {
        return Ignored;
    }

    hoverAmount.animateTo(1.0f, easeOutCubic<float>, 0.16f);
    return Handled;
}

auto OMDemiurgeDropDownSelector::processMouseExit(float, float) -> OMDemiurgeEventResult
{
    hoverAmount.animateTo(0.0f, easeOutCubic<float>, 0.16f);
    pressing = false;
    pressAmount.animateTo(0.0f, easeOutCubic<float>, 0.14f);
    return Ignored;
}

auto OMDemiurgeDropDownSelector::processMouseUp(float, float, uint8_t button) -> OMDemiurgeEventResult
{
    if (button == 1 && pressing)
    {
        pressing = false;
        pressAmount.animateTo(0.0f, easeOutCubic<float>, 0.14f);
    }

    return Ignored;
}

auto OMDemiurgeDropDownSelector::processMouseDown(float, float, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled.load() || button != 1)
    {
        return Ignored;
    }

    pressing = true;
    pressStarted = std::chrono::steady_clock::now();
    pressAmount.setTo(1.0f);

    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    // While a popup exists (even if closure is queued), a click always closes;
    // otherwise a click on the backdrop would close and immediately reopen it.
    if (opened.load() || popupLayer)
    {
        close();
    }
    else
    {
        open();
    }

    return Handled;
}

auto OMDemiurgeDropDownSelector::computePanelRect(const OMDemiurgeRect &bound, const OMDemiurgeRect &viewport)
    -> OMDemiurgeRect
{
    const float width = std::max(bound.width, 180.0f);
    const float minX = viewport.x + 4.0f;
    const float maxX = viewport.x + viewport.width - 4.0f;
    const float x = std::clamp(bound.x, minX, std::max(minX, maxX - width));

    const float minY = viewport.y + 4.0f;
    const float maxY = viewport.y + viewport.height - 4.0f;
    const float desired = std::max(44.0f, popupHeight);

    // Prefer below the header. If the full popup does not fit below but does
    // fit above, flip it; otherwise keep it below with a clamped, whole-row
    // height and let the list scroll.
    const float belowY = bound.y + bound.height + 4.0f;
    const float belowSpace = std::max(0.0f, maxY - belowY);
    const float aboveSpace = std::max(0.0f, bound.y - 4.0f - minY);

    float y = belowY;
    float height = std::min(desired, belowSpace);

    if (belowSpace < desired && aboveSpace >= desired)
    {
        height = desired;
        y = bound.y - 4.0f - height;
    }
    else if (height < 44.0f && aboveSpace > height)
    {
        height = std::min(desired, aboveSpace);
        y = bound.y - 4.0f - height;
    }

    if (height < 44.0f)
    {
        height = std::min(desired, std::max(44.0f, maxY - minY));
        y = std::clamp(y, minY, std::max(minY, maxY - height));
    }
    else if (height < desired)
    {
        height = std::min(desired, snapPopupHeight(height));
    }

    return OMDemiurgeRect{x, y, width, height};
}

void OMDemiurgeDropDownSelector::update()
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    if (pressing && opened.load() && std::chrono::steady_clock::now() - pressStarted > std::chrono::milliseconds(120))
    {
        pressing = false;
        pressAmount.animateTo(0.0f, easeOutCubic<float>, 0.16f);
    }

    const float hover = std::clamp(hoverAmount.get(), 0.0f, 1.0f);
    const float press = std::clamp(pressAmount.get(), 0.0f, 1.0f);
    glm::vec4 headerColor = glm::mix(normalColor, hoverColor, hover);
    headerColor = glm::mix(headerColor, pressedColor, press);
    background->style("color", (int)f4ToRgba(headerColor));

    const std::string text =
        selectedIndex >= 0 && selectedIndex < static_cast<int>(items.size()) ? items[selectedIndex] : placeholder;
    labelNode->style("text", text)->style("color", (int)(selectedIndex >= 0 ? 0xffffffff : 0xffffff9a));

    if (opened.load() && popupLayer)
    {
        try
        {
            auto bound = fetchLayoutBound();
            auto popupBound = popupLayer->fetchLayoutBound();

            lastPopupOriginX = popupBound.x;
            lastPopupOriginY = popupBound.y;
            lastPopupViewWidth = popupBound.width;
            lastPopupViewHeight = popupBound.height;

            const auto rect = computePanelRect(bound, popupBound);

            panel->style("offsetX", OMDemiurgeSize::pixels(rect.x - popupBound.x))
                ->style("offsetY", OMDemiurgeSize::pixels(rect.y - popupBound.y))
                ->style("width", OMDemiurgeSize::pixels(rect.width))
                ->style("height", OMDemiurgeSize::pixels(rect.height));

            OMDemiurgeScrollPane::setOverlayWheelRegion(static_cast<OMDemiurgeScrollPane *>(scrollPane.get()), rect);

            if (pendingScrollToSelection && scrollPane && selectedIndex >= 0 &&
                selectedIndex < static_cast<int>(itemNodes.size()))
            {
                try
                {
                    auto itemBound = itemNodes[selectedIndex]->fetchLayoutBound();
                    static_cast<OMDemiurgeScrollPane *>(scrollPane.get())->scrollRectIntoView(itemBound, true);
                    pendingScrollToSelection = false;
                }
                catch (const std::bad_any_cast &)
                {
                }
            }
            else if (selectedIndex < 0 || selectedIndex >= static_cast<int>(itemNodes.size()))
            {
                pendingScrollToSelection = false;
            }

            auto pane = static_cast<OMDemiurgeScrollPane *>(scrollPane.get());
            const float scrollY = pane->getScrollY();

            if (lastPopupScrollY >= 0.0f && std::abs(scrollY - lastPopupScrollY) > 0.01f)
            {
                // Content moved under the pointer: the rows no longer have valid
                // hover hit-testing until the next mouse move.
                for (auto &node : itemNodes)
                {
                    if (auto *item = dynamic_cast<SelectorItem *>(node.get()))
                    {
                        item->resetHover();
                    }
                }
            }

            lastPopupScrollY = scrollY;
        }
        catch (const std::bad_any_cast &)
        {
            // Popup or selector has not been laid out yet.
        }
    }
    else if (scrollPane)
    {
        OMDemiurgeScrollPane::clearOverlayWheelRegion(static_cast<OMDemiurgeScrollPane *>(scrollPane.get()));
    }

    OMDemiurgePane::update();
}

void OMDemiurgeDropDownSelector::remove()
{
    close();
    OMDemiurgePane::remove();
}

void OMDemiurgeDropDownSelector::ensurePopup()
{
    if (popupLayer)
    {
        return;
    }

    auto root = getRoot();

    if (root.get() == this)
    {
        return;
    }

    popupLayer = std::make_shared<SelectorPopupLayer>();
    popupLayer->style({{"position", OMDemiurgePosition::Absolute},
                       {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                       {"offsetY", OMDemiurgeSize::pixels(0.0f)},
                       {"width", OMDemiurgeSize::fill()},
                       {"height", OMDemiurgeSize::fill()}});
    popupLayer->setLayerOverlay(true);

    panel = std::make_shared<OMDemiurgeRectNode>();
    panel->style({{"position", OMDemiurgePosition::Absolute},
                  {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                  {"offsetY", OMDemiurgeSize::pixels(0.0f)},
                  {"width", OMDemiurgeSize::pixels(220.0f)},
                  {"height", OMDemiurgeSize::pixels(44.0f)},
                  {"radius", glm::vec4(8.0f)},
                  {"padding", OMDemiurgeEdgeInsets{4.0f, 4.0f, 4.0f, 4.0f}},
                  {"color", (int)f4ToRgba(glm::vec4(0.12f, 0.12f, 0.16f, 0.98f))}});

    auto pane = std::make_shared<OMDemiurgeScrollPane>();
    pane->style({{"width", OMDemiurgeSize::fill()}, {"height", OMDemiurgeSize::fill()}});
    pane->setOrientation(OMDemiurgeScrollPaneOrientation::Vertical);
    pane->setContentStretch(true);
    scrollPane = pane;
    panel->mountDirect(scrollPane);

    popupLayer->mountDirect(panel);

    auto back = std::make_shared<SelectorBackdrop>();
    back->style({{"position", OMDemiurgePosition::Absolute},
                 {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                 {"offsetY", OMDemiurgeSize::pixels(0.0f)},
                 {"width", OMDemiurgeSize::fill()},
                 {"height", OMDemiurgeSize::fill()},
                 {"color", 0x00000000}});
    back->onPress = [this, guard = alive]() {
        if (guard->load())
        {
            close();
        }
    };
    back->panel = panel;
    backdrop = back;
    popupLayer->mountDirect(backdrop);

    popupParent = root;

    // open() is usually called from processMouseDown; mutating the root's child
    // list during acceptEvent would invalidate its traversal iterator.
    auto layer = popupLayer;
    auto guard = alive;
    root->deferAction([root, layer, guard]() {
        if (guard->load())
        {
            root->mountFront(layer);
            root->setModalChild(layer);
        }
    });
}

void OMDemiurgeDropDownSelector::rebuildPopupContent()
{
    if (!scrollPane)
    {
        return;
    }

    itemNodes.clear();
    scrollContent = std::make_shared<OMDemiurgePane>();
    scrollContent->style({{"flexDirection", OMDemiurgeDirection::Column},
                          {"flexGap", OMDemiurgeSize::pixels(2.0f)},
                          {"width", OMDemiurgeSize::fill()}});

    for (int i = 0; i < static_cast<int>(items.size()); ++i)
    {
        auto item = std::make_shared<SelectorItem>(fontset);
        item->style({{"width", OMDemiurgeSize::fill()},
                     {"height", OMDemiurgeSize::pixels(ItemHeight)},
                     {"radius", glm::vec4(4.0f)},
                     {"label", items[i]},
                     {"textHeight", 12},
                     {"textMargin",
                      std::array<OMDemiurgeSize, 4>{OMDemiurgeSize::pixels(2.0f), OMDemiurgeSize::pixels(2.0f),
                                                    OMDemiurgeSize::pixels(8.0f), OMDemiurgeSize::pixels(8.0f)}}});
        item->setTextColor(glm::vec4(1.0f));

        if (i == selectedIndex)
        {
            item->setItemColors(glm::vec4(0.16f, 0.42f, 0.62f, 0.85f), glm::vec4(0.20f, 0.52f, 0.74f, 1.0f));
        }
        else
        {
            item->setItemColors(glm::vec4(0.16f, 0.16f, 0.20f, 0.0f), glm::vec4(0.30f, 0.40f, 0.52f, 0.55f));
        }

        // Select on release: Button only invokes this after a MouseUp inside.
        item->setOnClick([this, i, guard = alive]() {
            if (guard->load())
            {
                selectItem(i);
            }
        });
        scrollContent->mountDirect(item);
        itemNodes.push_back(item);
    }

    auto pane = std::static_pointer_cast<OMDemiurgeScrollPane>(scrollPane);
    pane->setContentNode(scrollContent);
}

void OMDemiurgeDropDownSelector::open()
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    if (disabled.load() || opened.load() || popupLayer)
    {
        return;
    }

    ensurePopup();

    if (!popupLayer)
    {
        return;
    }

    rebuildPopupContent();

    popupHeight =
        snapPopupHeight(std::max(44.0f, std::min(PanelPadding * 2.0f + ItemHeight * static_cast<float>(items.size()) +
                                                     std::max(0, static_cast<int>(items.size()) - 1) * ItemGap,
                                                 240.0f)));

    // Place the panel before the deferred mount so its first submitted frame is
    // not drawn at (0, 0). Reuse the last known popup viewport so the first frame
    // already has the final above/below choice (no vertical jump).
    try
    {
        auto bound = fetchLayoutBound();

        if (lastPopupViewWidth > 1.0f && lastPopupViewHeight > 1.0f)
        {
            const OMDemiurgeRect viewport{lastPopupOriginX, lastPopupOriginY, lastPopupViewWidth, lastPopupViewHeight};
            const auto rect = computePanelRect(bound, viewport);

            panel->style("offsetX", OMDemiurgeSize::pixels(rect.x - lastPopupOriginX))
                ->style("offsetY", OMDemiurgeSize::pixels(rect.y - lastPopupOriginY))
                ->style("width", OMDemiurgeSize::pixels(rect.width))
                ->style("height", OMDemiurgeSize::pixels(rect.height));
        }
        else
        {
            // No popup layout from a previous frame yet: derive the root content
            // box so even the first open already uses the final below/above spot.
            auto root = getRoot();
            auto rootBound = root->fetchLayoutBound();
            auto insets = root->fetchBorderInsets();
            const OMDemiurgeRect viewport{rootBound.x + insets.left, rootBound.y + insets.top,
                                          std::max(1.0f, rootBound.width - insets.left - insets.right),
                                          std::max(1.0f, rootBound.height - insets.top - insets.bottom)};

            lastPopupOriginX = viewport.x;
            lastPopupOriginY = viewport.y;
            lastPopupViewWidth = viewport.width;
            lastPopupViewHeight = viewport.height;

            const auto rect = computePanelRect(bound, viewport);
            panel->style("offsetX", OMDemiurgeSize::pixels(rect.x - viewport.x))
                ->style("offsetY", OMDemiurgeSize::pixels(rect.y - viewport.y))
                ->style("width", OMDemiurgeSize::pixels(rect.width))
                ->style("height", OMDemiurgeSize::pixels(rect.height));
        }
    }
    catch (const std::bad_any_cast &)
    {
        panel->style("height", OMDemiurgeSize::pixels(popupHeight));
    }

    opened.store(true);
    pendingScrollToSelection = true;
    lastPopupScrollY = -1.0f;
    requestFocus();
}

void OMDemiurgeDropDownSelector::close()
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    opened.store(false);

    if (!popupLayer || closing)
    {
        return;
    }

    closing = true;
    lastPopupScrollY = -1.0f;

    if (scrollPane)
    {
        OMDemiurgeScrollPane::clearOverlayWheelRegion(static_cast<OMDemiurgeScrollPane *>(scrollPane.get()));
    }

    auto parent = popupParent.lock();
    auto layer = popupLayer;
    auto guard = alive;

    if (parent)
    {
        // Keep popupLayer non-null until the deferred removal runs; a second
        // click in the same event batch must not create a new popup.
        parent->deferAction([this, parent, layer, guard]() {
            parent->clearModalChild(layer);
            parent->umount(layer);

            if (!guard->load())
            {
                return;
            }

            std::lock_guard<std::recursive_mutex> lock(stateMutex);

            if (popupLayer == layer)
            {
                popupLayer.reset();
                panel.reset();
                backdrop.reset();
                scrollPane.reset();
                scrollContent.reset();
                popupParent.reset();
                closing = false;
            }
        });
    }
    else
    {
        popupLayer.reset();
        panel.reset();
        backdrop.reset();
        scrollPane.reset();
        scrollContent.reset();
        popupParent.reset();
        closing = false;
    }
}

void OMDemiurgeDropDownSelector::selectItem(int index)
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    if (index < 0 || index >= static_cast<int>(items.size()))
    {
        return;
    }

    const bool changed = selectedIndex != index;
    selectedIndex = index;
    close();

    if (changed && onSelect)
    {
        onSelect(index, items[index]);
    }
}

void OMDemiurgeDropDownSelector::setItems(const std::vector<std::string> &values)
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    items = values;

    if (selectedIndex >= static_cast<int>(items.size()))
    {
        selectedIndex = items.empty() ? -1 : static_cast<int>(items.size()) - 1;
    }

    if (opened.load())
    {
        rebuildPopupContent();
        popupHeight = snapPopupHeight(
            std::max(44.0f, std::min(PanelPadding * 2.0f + ItemHeight * static_cast<float>(items.size()) +
                                         std::max(0, static_cast<int>(items.size()) - 1) * ItemGap,
                                     240.0f)));
        panel->style("height", OMDemiurgeSize::pixels(popupHeight));
    }
}

void OMDemiurgeDropDownSelector::setSelectedIndex(int index, bool notify)
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    const int next = index >= 0 && index < static_cast<int>(items.size()) ? index : -1;

    if (next == selectedIndex)
    {
        return;
    }

    selectedIndex = next;

    if (notify && selectedIndex >= 0 && onSelect)
    {
        onSelect(selectedIndex, items[selectedIndex]);
    }
}

auto OMDemiurgeDropDownSelector::getSelectedIndex() const -> int
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    return selectedIndex;
}

auto OMDemiurgeDropDownSelector::getSelectedValue() const -> std::string
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    return selectedIndex >= 0 && selectedIndex < static_cast<int>(items.size()) ? items[selectedIndex] : std::string();
}

void OMDemiurgeDropDownSelector::setHeaderColors(const glm::vec4 &normal, const glm::vec4 &hovered,
                                                 const glm::vec4 &pressed)
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    normalColor = normal;
    hoverColor = hovered;
    pressedColor = pressed;
}

void OMDemiurgeDropDownSelector::setPlaceholder(const std::string &value)
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    placeholder = value;
}

void OMDemiurgeDropDownSelector::setOnSelect(std::function<void(int, const std::string &)> handler)
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    onSelect = std::move(handler);
}

void OMDemiurgeDropDownSelector::setDisabled(bool value, bool)
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    disabled.store(value);

    if (disabled.load())
    {
        close();
    }
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
