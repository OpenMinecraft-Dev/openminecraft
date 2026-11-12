#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_tooltip.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "fmt/format.h"
#include "boost/core/demangle.hpp"
#include <algorithm>
#include <typeinfo>
#include <vector>
#include "yoga/YGNode.h"
#include "yoga/YGNodeStyle.h"
#include "yoga/YGValue.h"
#include <any>
#include <array>
#include <cmath>
#include <iostream>

namespace openminecraft::renderer::common::demiurge
{
static auto toYGDirection(OMDemiurgeDirection d) -> YGFlexDirection
{
    switch (d)
    {
    case Row:
    default:
        return YGFlexDirectionRow;
    case RowReverse:
        return YGFlexDirectionRowReverse;
    case Column:
        return YGFlexDirectionColumn;
    case ColumnReverse:
        return YGFlexDirectionColumnReverse;
    }
}

static auto toYGWrap(OMDemiurgeWrap w) -> YGWrap
{
    switch (w)
    {
    case Wrap:
        return YGWrapWrap;
    case WrapReverse:
        return YGWrapWrapReverse;
    default:
    case NoWrap:
        return YGWrapNoWrap;
    }
}

static auto toYGJustify(OMDemiurgeAlign a) -> YGJustify
{
    switch (a)
    {
    default:
    case FlexStart:
        return YGJustifyFlexStart;
    case Auto:
    case Stretch:
    case Baseline:
    case Center:
        return YGJustifyCenter;
    case FlexEnd:
        return YGJustifyFlexEnd;
    case SpaceAround:
        return YGJustifySpaceAround;
    case SpaceBetween:
        return YGJustifySpaceBetween;
    case SpaceEvenly:
        return YGJustifySpaceEvenly;
    }
}
static auto toYGAlign(OMDemiurgeAlign a) -> YGAlign
{
    switch (a)
    {
    default:
    case Auto:
        return YGAlignAuto;
    case FlexStart:
        return YGAlignFlexStart;
    case Center:
        return YGAlignCenter;
    case FlexEnd:
        return YGAlignFlexEnd;
    case Stretch:
        return YGAlignStretch;
    case Baseline:
        return YGAlignBaseline;
    case SpaceBetween:
        return YGAlignSpaceBetween;
    case SpaceAround:
        return YGAlignSpaceAround;
    case SpaceEvenly:
        return YGAlignSpaceEvenly;
    }
}

static void setSizeToYoga(YGNodeRef node, OMDemiurgeSize size, bool isWidth)
{
    switch (size.unit)
    {
    case OMDemiurgeSize::Pixel:
        if (isWidth)
            YGNodeStyleSetWidth(node, size.value);
        else
            YGNodeStyleSetHeight(node, size.value);
        break;
    case OMDemiurgeSize::Percent:
        if (isWidth)
            YGNodeStyleSetWidthPercent(node, size.value * 100.0f);
        else
            YGNodeStyleSetHeightPercent(node, size.value * 100.0f);
        break;
    case OMDemiurgeSize::Fit:
        if (isWidth)
            YGNodeStyleSetWidthAuto(node);
        else
            YGNodeStyleSetHeightAuto(node);
        break;
    }
}

OMDemiurgeNode::OMDemiurgeNode()
{
    yogaNode = YGNodeNew();
}
OMDemiurgeNode::~OMDemiurgeNode()
{
    node::controls::OMDemiurgeTooltip::instance().detach(this);
    YGNodeFree(yogaNode);
}

auto OMDemiurgeNode::syncLayout() -> void
{
    if (stylesStorage.isModified())
    {
        using namespace openminecraft::binary::hash;
        for (auto &p : stylesStorage)
        {
            switch (hash_compile_time(p.first.c_str()))
            {
            case "minWidth"_hash:
                YGNodeStyleSetMinWidth(yogaNode, std::any_cast<float>(p.second));
                break;
            case "minHeight"_hash:
                YGNodeStyleSetMinHeight(yogaNode, std::any_cast<float>(p.second));
                break;
            case "maxWidth"_hash:
                YGNodeStyleSetMaxWidth(yogaNode, std::any_cast<float>(p.second));
                break;
            case "maxHeight"_hash:
                YGNodeStyleSetMaxHeight(yogaNode, std::any_cast<float>(p.second));
                break;
            case "width"_hash:
                setSizeToYoga(yogaNode, std::any_cast<OMDemiurgeSize>(p.second), true);
                break;
            case "height"_hash:
                setSizeToYoga(yogaNode, std::any_cast<OMDemiurgeSize>(p.second), false);
                break;
            case "alignItems"_hash:
                YGNodeStyleSetAlignItems(yogaNode, toYGAlign(std::any_cast<OMDemiurgeAlign>(p.second)));
                break;
            case "alignContent"_hash:
                YGNodeStyleSetAlignContent(yogaNode, toYGAlign(std::any_cast<OMDemiurgeAlign>(p.second)));
                break;
            case "justifyContent"_hash:
                YGNodeStyleSetJustifyContent(yogaNode, toYGJustify(std::any_cast<OMDemiurgeAlign>(p.second)));
                break;
            case "alignSelf"_hash:
                YGNodeStyleSetAlignSelf(yogaNode, toYGAlign(std::any_cast<OMDemiurgeAlign>(p.second)));
                break;
            case "flexBasis"_hash: {
                auto s = std::any_cast<OMDemiurgeSize>(p.second);
                switch (s.unit)
                {
                case OMDemiurgeSize::Pixel:
                    YGNodeStyleSetFlexBasis(yogaNode, s.value);
                    break;
                case OMDemiurgeSize::Percent:
                    YGNodeStyleSetFlexBasisPercent(yogaNode, s.value * 100.0f);
                    break;
                default:
                    YGNodeStyleSetFlexBasisAuto(yogaNode);
                    break;
                }
                break;
            }
            case "flexGrow"_hash:
                YGNodeStyleSetFlexGrow(yogaNode, std::any_cast<float>(p.second));
                break;
            case "flexShrink"_hash:
                YGNodeStyleSetFlexShrink(yogaNode, std::any_cast<float>(p.second));
                break;
            case "flexGap"_hash: {
                auto g = std::any_cast<OMDemiurgeSize>(p.second);
                if (g.unit == OMDemiurgeSize::Percent)
                {
                    YGNodeStyleSetGapPercent(yogaNode, YGGutterAll, g.value * 100.0f);
                }
                else
                {
                    YGNodeStyleSetGap(yogaNode, YGGutterAll, g.value);
                }
                break;
            }
            case "offsetY"_hash:
            case "offsetX"_hash: {
                auto edge = p.first == "offsetX" ? YGEdgeLeft : YGEdgeTop;
                auto s = std::any_cast<OMDemiurgeSize>(p.second);
                switch (s.unit)
                {
                case OMDemiurgeSize::Pixel:
                    YGNodeStyleSetPosition(yogaNode, edge, s.value);
                    break;
                case OMDemiurgeSize::Percent:
                    YGNodeStyleSetPositionPercent(yogaNode, edge, s.value * 100.0f);
                    break;
                case OMDemiurgeSize::Fit:
                    YGNodeStyleSetPositionAuto(yogaNode, edge);
                    break;
                }
                break;
            }
            case "position"_hash: {
                YGNodeStyleSetPositionType(yogaNode, std::any_cast<OMDemiurgePosition>(p.second) == Absolute
                                                         ? YGPositionTypeAbsolute
                                                         : YGPositionTypeRelative);
                break;
            }
            case "flexDirection"_hash: {
                YGNodeStyleSetFlexDirection(yogaNode, toYGDirection(std::any_cast<OMDemiurgeDirection>(p.second)));
                break;
            }
            case "flexWrap"_hash: {
                YGNodeStyleSetFlexWrap(yogaNode, toYGWrap(std::any_cast<OMDemiurgeWrap>(p.second)));
                break;
            }
            case "margin"_hash: {
                auto insets = std::any_cast<std::array<OMDemiurgeSize, 4>>(p.second);
                auto &top = insets[0];
                auto &bottom = insets[1];
                auto &left = insets[2];
                auto &right = insets[3];

#define update(source, type)                                                                                           \
    switch (source.unit)                                                                                               \
    {                                                                                                                  \
    case OMDemiurgeSize::Pixel:                                                                                        \
        YGNodeStyleSetMargin(yogaNode, YGEdge##type, source.value);                                                    \
        break;                                                                                                         \
    case OMDemiurgeSize::Percent:                                                                                      \
        YGNodeStyleSetMarginPercent(yogaNode, YGEdge##type, source.value * 100.0f);                                    \
        break;                                                                                                         \
    default:                                                                                                           \
        YGNodeStyleSetMarginAuto(yogaNode, YGEdge##type);                                                              \
        break;                                                                                                         \
    }

                update(top, Top);
                update(bottom, Bottom);
                update(left, Left);
                update(right, Right);
                break;
            }
            case "border"_hash: {
                auto insets = std::any_cast<OMDemiurgeEdgeInsets>(p.second);
                YGNodeStyleSetBorder(yogaNode, YGEdgeTop, insets.top);
                YGNodeStyleSetBorder(yogaNode, YGEdgeBottom, insets.bottom);
                YGNodeStyleSetBorder(yogaNode, YGEdgeLeft, insets.left);
                YGNodeStyleSetBorder(yogaNode, YGEdgeRight, insets.right);
                break;
            }
            case "padding"_hash: {
                auto insets = std::any_cast<OMDemiurgeEdgeInsets>(p.second);
                YGNodeStyleSetPadding(yogaNode, YGEdgeTop, insets.top);
                YGNodeStyleSetPadding(yogaNode, YGEdgeBottom, insets.bottom);
                YGNodeStyleSetPadding(yogaNode, YGEdgeLeft, insets.left);
                YGNodeStyleSetPadding(yogaNode, YGEdgeRight, insets.right);
                break;
            }
            }
        }
    }
}

auto OMDemiurgeNode::syncLayoutAll() -> void
{
    syncLayout();
    for (auto f : children)
    {
        f->syncLayoutAll();
    }
}

void OMDemiurgeNode::layout(float width, float height)
{
    syncLayoutAll();
    YGNodeCalculateLayout(yogaNode, width, height, YGDirectionLTR);
    syncBoundary(0, 0);
}

void OMDemiurgeNode::refreshHover()
{
    // While a control owns the mouse, hover must not be re-synthesized: the
    // captured node is already receiving the real move stream.
    if (fetchMouseCapture())
    {
        return;
    }

    if (!std::isfinite(mx) || !std::isfinite(my))
    {
        return;
    }

    OMDemiurgeEvent event;
    event.type = OMDemiurgeEventType::MouseMove;
    event.x = mx;
    event.y = my;
    acceptEvent(event);
}

auto OMDemiurgeNode::syncBoundary(float x, float y) -> void
{
    auto b = boundary();
    b.x += x;
    b.y += y;
    try
    {
        auto bo = stylesStorage.get<OMDemiurgeRect>("layoutBound");
        if (b.x == bo.x && b.y == bo.y && b.width == bo.width && b.height == bo.height)
        {
            goto updateChildren;
        }
    }
    catch (std::bad_any_cast &e)
    {
    }

    stylesStorage.put("layoutBound", b);

updateChildren:
    for (auto f : children)
    {
        f->syncBoundary(b.x, b.y);
    }
}

auto OMDemiurgeNode::resolveClipBound() -> std::optional<OMDemiurgeRect>
{
    std::optional<OMDemiurgeRect> bound;

    for (auto cur = this; cur != nullptr; cur = cur->parent)
    {
        auto clip = cur->clipNodeRef.lock();

        if (!clip)
        {
            continue;
        }

        try
        {
            const auto b = clip->fetchLayoutBound();

            if (!bound.has_value())
            {
                bound = b;
            }
            else
            {
                const float x0 = std::max(bound->x, b.x);
                const float y0 = std::max(bound->y, b.y);
                const float x1 = std::min(bound->x + bound->width, b.x + b.width);
                const float y1 = std::min(bound->y + bound->height, b.y + b.height);

                bound->x = x0;
                bound->y = y0;
                bound->width = std::max(0.0f, x1 - x0);
                bound->height = std::max(0.0f, y1 - y0);
            }
        }
        catch (std::bad_any_cast &)
        {
            // Clip node not laid out yet: ignore it.
        }
    }

    return bound;
}

void OMDemiurgeNode::setFocusable(bool value)
{
    focusable = value;
}

void OMDemiurgeNode::requestFocus()
{
    auto self = shared_from_this();
    auto previous = focusedNode.lock();

    if (previous.get() == this)
    {
        return;
    }

    auto buildChain = [](std::shared_ptr<OMDemiurgeNode> cur) -> std::vector<OMDemiurgeNode *> {
        std::vector<OMDemiurgeNode *> chain;

        while (cur)
        {
            chain.push_back(cur.get());
            cur = cur->parent ? cur->parent->shared_from_this() : nullptr;
        }

        return chain;
    };

    auto oldChain = buildChain(previous);
    auto newChain = buildChain(self);

    auto inChain = [](const std::vector<OMDemiurgeNode *> &chain, OMDemiurgeNode *node) -> bool {
        return std::find(chain.begin(), chain.end(), node) != chain.end();
    };

    // Losing nodes: leaf -> root.
    for (auto *node : oldChain)
    {
        if (!inChain(newChain, node))
        {
            node->processFocusLost();
        }
    }

    // Gaining nodes: root -> leaf, so ancestors react before their children.
    for (auto it = newChain.rbegin(); it != newChain.rend(); ++it)
    {
        if (!inChain(oldChain, *it))
        {
            (*it)->processFocusGained();
        }
    }

    focusedNode = self;
    focusedRoot = newChain.back()->shared_from_this();
}

void OMDemiurgeNode::releaseFocus()
{
    auto current = focusedNode.lock();

    if (current.get() != this)
    {
        return;
    }

    auto cur = current;

    while (cur)
    {
        cur->processFocusLost();
        cur = cur->parent ? cur->parent->shared_from_this() : nullptr;
    }

    focusedNode.reset();
    focusedRoot.reset();
}

auto OMDemiurgeNode::setTooltip(const std::string &text, float delay) -> std::shared_ptr<OMDemiurgeNode>
{
    node::controls::OMDemiurgeTooltip::instance().setTooltip(this, text, delay);
    return shared_from_this();
}

auto OMDemiurgeNode::setImmediateTooltip(const std::string &text) -> std::shared_ptr<OMDemiurgeNode>
{
    node::controls::OMDemiurgeTooltip::instance().setImmediateTooltip(this, text);
    return shared_from_this();
}

auto OMDemiurgeNode::getTooltip() const -> std::string
{
    return node::controls::OMDemiurgeTooltip::instance().getTooltip(this);
}

void OMDemiurgeNode::setTooltipDelay(float seconds)
{
    node::controls::OMDemiurgeTooltip::instance().setTooltipDelay(this, seconds);
}

auto OMDemiurgeNode::getTooltipDelay() const -> float
{
    return node::controls::OMDemiurgeTooltip::instance().getTooltipDelay(this);
}

void OMDemiurgeNode::hideTooltip()
{
    node::controls::OMDemiurgeTooltip::instance().hideFor(this);
}

auto OMDemiurgeNode::isTooltipHovered() const -> bool
{
    return node::controls::OMDemiurgeTooltip::instance().isHovered(this);
}

auto OMDemiurgeNode::setChildId(const std::shared_ptr<OMDemiurgeNode> &child, const std::string &id) -> bool
{
    if (!child || std::find(children.begin(), children.end(), child) == children.end())
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(childIdMutex);

    if (id.empty())
    {
        auto old = childIdByNode.find(child.get());

        if (old != childIdByNode.end())
        {
            childById.erase(old->second);
            childIdByNode.erase(old);
        }

        return true;
    }

    if (auto existing = childById.find(id); existing != childById.end())
    {
        auto holder = existing->second.lock();

        if (holder && holder != child)
        {
            return false;
        }

        if (!holder)
        {
            childById.erase(existing);
        }
    }

    if (auto old = childIdByNode.find(child.get()); old != childIdByNode.end())
    {
        childById.erase(old->second);
        childIdByNode.erase(old);
    }

    childIdByNode[child.get()] = id;
    childById[id] = child;
    return true;
}

void OMDemiurgeNode::clearChildId(const std::shared_ptr<OMDemiurgeNode> &child)
{
    if (!child)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(childIdMutex);
    auto old = childIdByNode.find(child.get());

    if (old != childIdByNode.end())
    {
        childById.erase(old->second);
        childIdByNode.erase(old);
    }
}

auto OMDemiurgeNode::getChildId(const OMDemiurgeNode *child) const -> std::optional<std::string>
{
    if (child == nullptr)
    {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(childIdMutex);
    auto it = childIdByNode.find(child);
    return it == childIdByNode.end() ? std::nullopt : std::optional<std::string>(it->second);
}

auto OMDemiurgeNode::findChildById(const std::string &id) -> std::shared_ptr<OMDemiurgeNode>
{
    std::lock_guard<std::mutex> lock(childIdMutex);
    auto it = childById.find(id);

    if (it == childById.end())
    {
        return nullptr;
    }

    auto child = it->second.lock();

    if (!child)
    {
        childById.erase(it);
        return nullptr;
    }

    return child;
}

void OMDemiurgeNode::releaseFocusWithin(const std::shared_ptr<OMDemiurgeNode> &root)
{
    if (!root)
    {
        return;
    }

    auto focused = focusedNode.lock();

    for (auto cur = focused; cur; cur = cur->parent ? cur->parent->shared_from_this() : nullptr)
    {
        if (cur == root)
        {
            focused->releaseFocus();
            return;
        }
    }
}

auto OMDemiurgeNode::hasFocus() -> bool
{
    auto cur = focusedNode.lock();

    while (cur)
    {
        if (cur.get() == this)
        {
            return true;
        }

        cur = cur->parent ? cur->parent->shared_from_this() : nullptr;
    }

    return false;
}

void OMDemiurgeNode::captureMouse()
{
    auto self = shared_from_this();
    auto root = self;

    while (root->parent != nullptr)
    {
        root = root->parent->shared_from_this();
    }

    capturedNode = self;
    capturedRoot = root;
}

void OMDemiurgeNode::releaseMouse()
{
    if (auto current = capturedNode.lock(); current.get() == this)
    {
        capturedNode.reset();
        capturedRoot.reset();
    }
}

auto OMDemiurgeNode::hasMouseCapture() -> bool
{
    return capturedNode.lock().get() == this;
}

auto OMDemiurgeNode::containsPoint(float x, float y) -> bool
{
    if (!visible)
    {
        return false;
    }

    try
    {
        auto bo = stylesStorage.get<OMDemiurgeRect>("layoutBound");
        return x >= bo.x && x <= bo.x + bo.width && y >= bo.y && y <= bo.y + bo.height;
    }
    catch (std::bad_any_cast &e)
    {
        return false;
    }
}

auto OMDemiurgeNode::isHoverableAt(float x, float y) -> bool
{
    for (auto cur = this; cur != nullptr; cur = cur->parent)
    {
        if (!cur->visible)
        {
            return false;
        }
    }

    if (!containsPoint(x, y))
    {
        return false;
    }

    if (auto clip = resolveClipBound(); clip.has_value())
    {
        return x >= clip->x && x <= clip->x + clip->width && y >= clip->y && y <= clip->y + clip->height;
    }

    return true;
}

auto OMDemiurgeNode::dispatchCapturedEvent(const OMDemiurgeEvent &event) -> void
{
    if (event.x != INFINITY && event.y != INFINITY)
    {
        mx = event.x;
        my = event.y;
    }

    processEvent(event);
}

void OMDemiurgeNode::acceptEvent(const OMDemiurgeEvent &event)
{
    if (!visible)
    {
        return;
    }

    if (event.isMouse() && std::isfinite(event.x) && std::isfinite(event.y))
    {
        // Update even while a child holds the mouse capture; hover refresh and
        // captured drags must see the latest pointer, not the press position.
        mx = event.x;
        my = event.y;
    }

    if (parent == nullptr && event.isMouse())
    {
        node::controls::OMDemiurgeTooltip::instance().setPointer(event.x, event.y);
    }

    if (event.type == OMDemiurgeEventType::MouseDown)
    {
        node::controls::OMDemiurgeTooltip::instance().dismiss();
    }
    else if (event.type == OMDemiurgeEventType::MouseUp)
    {
        node::controls::OMDemiurgeTooltip::instance().resume();
    }

    std::shared_ptr<OMDemiurgeNode> focusBefore;
    std::shared_ptr<OMDemiurgeNode> modal;

    if (parent == nullptr && event.isMouse())
    {
        std::lock_guard<std::mutex> lock(modalMutex);
        modal = modalChild.lock();
    }

    if (parent == nullptr && event.type == OMDemiurgeEventType::MouseDown)
    {
        focusBefore = focusedNode.lock();
    }

    // Mouse capture takes over the whole tree while a control is held down.
    if (parent == nullptr && event.isMouse())
    {
        if (auto captured = capturedNode.lock())
        {
            auto root = capturedRoot.lock();

            if (root.get() == this)
            {
                switch (event.type)
                {
                case OMDemiurgeEventType::MouseDown: {
                    // A new press cancels the previous capture.
                    OMDemiurgeEvent exit = event;
                    exit.type = OMDemiurgeEventType::MouseExit;
                    captured->dispatchCapturedEvent(exit);
                    capturedNode.reset();
                    capturedRoot.reset();
                    break;
                }
                case OMDemiurgeEventType::MouseMove: {
                    // Give ancestors (e.g. ScrollPane) a chance to steal the drag.
                    for (auto ancestor = captured->parent; ancestor != nullptr; ancestor = ancestor->parent)
                    {
                        if (ancestor->processNestedDrag(captured, event) == Handled)
                        {
                            return;
                        }
                    }

                    captured->dispatchCapturedEvent(event);
                    return;
                }
                case OMDemiurgeEventType::MouseUp: {
                    captured->dispatchCapturedEvent(event);

                    const bool inside = captured->containsPoint(event.x, event.y);

                    capturedNode.reset();
                    capturedRoot.reset();

                    if (!inside)
                    {
                        OMDemiurgeEvent exit = event;
                        exit.type = OMDemiurgeEventType::MouseExit;
                        captured->dispatchCapturedEvent(exit);
                    }

                    return;
                }
                default:
                    break;
                }
            }
        }
    }

    if (parent == nullptr && event.isMouse() && modal && modal.get() != this)
    {
        modal->acceptEvent(event);
        return;
    }

    if (event.isKeyboard())
    {
        if (parent == nullptr)
        {
            auto focused = focusedNode.lock();
            auto root = focusedRoot.lock();

            if (focused && root.get() == this)
            {
                // Walk focus chain from the focused node up to the root.
                auto cur = focused;

                while (cur)
                {
                    if (cur->processEvent(event) == Handled)
                    {
                        return;
                    }

                    cur = cur->parent ? cur->parent->shared_from_this() : nullptr;
                }

                return;
            }
        }

        if (processEvent(event) == Ignored)
        {
            for (auto f : children)
            {
                f->acceptEvent(event);
            }
        }

        return;
    }

    try
    {
        auto bo = stylesStorage.get<OMDemiurgeRect>("layoutBound");

        const bool inside =
            (event.x >= bo.x && event.x <= bo.x + bo.width && event.y >= bo.y && event.y <= bo.y + bo.height) ||
            (std::isinf(event.x) && std::isinf(event.y));
        // Keep a separate hover flag: comparing mx/my against the *new* bounds
        // fails when layout moves a node under a stationary pointer.
        const bool wasInside = hoverInside;

        if (inside)
        {
            if (!wasInside)
            {
                hoverInside = true;
                node::controls::OMDemiurgeTooltip::instance().onHover(this, true, event.x, event.y);

                OMDemiurgeEvent enter = event;
                enter.type = OMDemiurgeEventType::MouseEnter;

                if (processEvent(enter) == Ignored)
                {
                    for (auto f : children)
                    {
                        f->acceptEvent(enter);
                    }
                }
            }

            const auto result = processEvent(event);

            if (result == OMDemiurgeEventResult::Handled && event.type == OMDemiurgeEventType::MouseDown && focusable)
            {
                requestFocus();
            }

            if (result == Ignored)
            {
                for (auto f : children)
                {
                    f->acceptEvent(event);
                }
            }
        }
        else if (wasInside)
        {
            hoverInside = false;
            node::controls::OMDemiurgeTooltip::instance().onHover(this, false, event.x, event.y);

            OMDemiurgeEvent exit = event;
            exit.type = OMDemiurgeEventType::MouseExit;

            if (processEvent(exit) == Ignored)
            {
                for (auto f : children)
                {
                    f->acceptEvent(exit);
                }
            }
        }

        if (event.x != INFINITY && event.y != INFINITY)
        {
            mx = event.x;
            my = event.y;
        }

        // Clicking outside the focused node (or on a non-focusable element) drops
        // focus. Focus changes triggered during this dispatch keep the new node.
        if (parent == nullptr && event.type == OMDemiurgeEventType::MouseDown && focusBefore &&
            focusedNode.lock() == focusBefore && !focusBefore->containsPoint(event.x, event.y))
        {
            focusBefore->releaseFocus();
        }
    }
    catch (std::bad_any_cast &e)
    {
    }
}

} // namespace openminecraft::renderer::common::demiurge

namespace openminecraft::renderer::common::demiurge
{
static auto demangleName(const char *name) -> std::string
{
    std::string full = boost::core::demangle(name);
    const auto pos = full.rfind("::");

    if (pos != std::string::npos)
    {
        return full.substr(pos + 2);
    }

    return full;
}

static auto sizeToString(const OMDemiurgeSize &size) -> std::string
{
    switch (size.unit)
    {
    case OMDemiurgeSize::Pixel:
        return fmt::format("px({:.1f})", size.value);
    case OMDemiurgeSize::Percent:
        return fmt::format("fill({:.0f}%)", size.value * 100.0f);
    case OMDemiurgeSize::Fit:
    default:
        return "fit";
    }
}

static auto styleValueToString(const std::any &value) -> std::string
{
    if (!value.has_value())
    {
        return "null";
    }

    const auto &type = value.type();

    if (type == typeid(OMDemiurgeSize))
    {
        return sizeToString(std::any_cast<OMDemiurgeSize>(value));
    }

    if (type == typeid(std::array<OMDemiurgeSize, 4>))
    {
        const auto v = std::any_cast<std::array<OMDemiurgeSize, 4>>(value);
        return fmt::format("insets4(t={}, b={}, l={}, r={})", sizeToString(v[0]), sizeToString(v[1]),
                           sizeToString(v[2]), sizeToString(v[3]));
    }

    if (type == typeid(OMDemiurgeRect))
    {
        const auto rect = std::any_cast<OMDemiurgeRect>(value);
        return fmt::format("rect({:.1f}, {:.1f}, {:.1f}, {:.1f})", rect.x, rect.y, rect.width, rect.height);
    }

    if (type == typeid(OMDemiurgeEdgeInsets))
    {
        const auto in = std::any_cast<OMDemiurgeEdgeInsets>(value);
        return fmt::format("insets(t={:.1f}, b={:.1f}, l={:.1f}, r={:.1f})", in.top, in.bottom, in.left, in.right);
    }

    if (type == typeid(glm::vec4))
    {
        const auto v = std::any_cast<glm::vec4>(value);
        return fmt::format("vec4({:.3f}, {:.3f}, {:.3f}, {:.3f})", v.x, v.y, v.z, v.w);
    }

    if (type == typeid(glm::vec3))
    {
        const auto v = std::any_cast<glm::vec3>(value);
        return fmt::format("vec3({:.3f}, {:.3f}, {:.3f})", v.x, v.y, v.z);
    }

    if (type == typeid(glm::vec2))
    {
        const auto v = std::any_cast<glm::vec2>(value);
        return fmt::format("vec2({:.3f}, {:.3f})", v.x, v.y);
    }

    if (type == typeid(OMDemiurgePosition))
    {
        return std::any_cast<OMDemiurgePosition>(value) == Absolute ? "Absolute" : "Relative";
    }

    if (type == typeid(OMDemiurgeDirection))
    {
        switch (std::any_cast<OMDemiurgeDirection>(value))
        {
        case Row:
            return "Row";
        case RowReverse:
            return "RowReverse";
        case Column:
            return "Column";
        case ColumnReverse:
            return "ColumnReverse";
        }
    }

    if (type == typeid(OMDemiurgeWrap))
    {
        switch (std::any_cast<OMDemiurgeWrap>(value))
        {
        case Wrap:
            return "Wrap";
        case WrapReverse:
            return "WrapReverse";
        case NoWrap:
            return "NoWrap";
        }
    }

    if (type == typeid(OMDemiurgeAlign))
    {
        switch (std::any_cast<OMDemiurgeAlign>(value))
        {
        case Auto:
            return "Auto";
        case FlexStart:
            return "FlexStart";
        case Center:
            return "Center";
        case FlexEnd:
            return "FlexEnd";
        case Stretch:
            return "Stretch";
        case Baseline:
            return "Baseline";
        case SpaceBetween:
            return "SpaceBetween";
        case SpaceAround:
            return "SpaceAround";
        case SpaceEvenly:
            return "SpaceEvenly";
        }
    }

    if (type == typeid(int))
    {
        return fmt::format("{}", std::any_cast<int>(value));
    }

    if (type == typeid(uint32_t))
    {
        return fmt::format("{}", std::any_cast<uint32_t>(value));
    }

    if (type == typeid(float))
    {
        return fmt::format("{:.3f}", std::any_cast<float>(value));
    }

    if (type == typeid(bool))
    {
        return std::any_cast<bool>(value) ? "true" : "false";
    }

    if (type == typeid(std::string))
    {
        return fmt::format("\"{}\"", std::any_cast<std::string>(value));
    }

    if (type == typeid(const char *))
    {
        const char *str = std::any_cast<const char *>(value);
        return fmt::format("\"{}\"", str != nullptr ? str : "");
    }

    return fmt::format("<{}>", demangleName(type.name()));
}

static auto stylesToString(const OMDemiurgeStyles &styles) -> std::string
{
    std::vector<std::pair<std::string, std::string>> entries;

    for (const auto &entry : styles)
    {
        entries.emplace_back(entry.first, styleValueToString(entry.second));
    }

    std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

    std::string out = "{";

    for (size_t i = 0; i < entries.size(); ++i)
    {
        if (i != 0)
        {
            out += ", ";
        }

        out += entries[i].first;
        out += "=";
        out += entries[i].second;
    }

    out += "}";
    return out;
}

static auto dumpNode(const OMDemiurgeNode &node, int depth, log::OMLogger &logger) -> void
{
    logger.info("{}{} {}", std::string(static_cast<size_t>(depth) * 2, ' '), node.typeName(), node.styleDump());

    for (const auto &child : node.fetchChildren())
    {
        if (child)
        {
            dumpNode(*child, depth + 1, logger);
        }
    }
}

auto OMDemiurgeNode::typeName() const -> std::string
{
    return demangleName(typeid(*this).name());
}

auto OMDemiurgeNode::styleDump() const -> std::string
{
    return stylesToString(stylesStorage);
}

auto OMDemiurgeNode::dumpTree() const -> void
{
    log::OMLogger logger("demiurge.ui");
    logger.info("UI tree:");
    dumpNode(*this, 0, logger);
}
} // namespace openminecraft::renderer::common::demiurge
