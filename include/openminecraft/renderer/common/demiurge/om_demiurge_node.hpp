#ifndef OM_DEMIURGE_NODE_HPP
#define OM_DEMIURGE_NODE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_layer.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_event.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_styles.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/event/om_eventbus_wrap.hpp"
#include "yoga/YGNode.h"
#include "yoga/YGNodeLayout.h"
#include <algorithm>
#include <functional>
#include <mutex>
#include <any>
#include <cmath>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <string>
#include <utility>
#include <vector>
#include <yoga/Yoga.h>
#include <cstdint>
#include <array>
#include <chrono>

namespace openminecraft::geom
{
class OMFontSet;
}

namespace openminecraft::renderer::common::demiurge
{
class OMDemiurgeRendererHandler;
enum OMDemiurgeEventResult
{
    Handled,
    Ignored,
};
class OMDemiurgeNode : public std::enable_shared_from_this<OMDemiurgeNode>
{
  public:
    OMDemiurgeNode();
    ~OMDemiurgeNode();

    virtual auto store(std::shared_ptr<OMDemiurgeNode> &target) -> std::shared_ptr<OMDemiurgeNode>
    {
        target = shared_from_this();
        return shared_from_this();
    }

    virtual auto mount(std::shared_ptr<OMDemiurgeNode> child) -> std::shared_ptr<OMDemiurgeNode>
    {
        child->parent = this;
        children.push_back(child);

        YGNodeInsertChild(yogaNode, child->yogaNode, children.size() - 1);

        return shared_from_this();
    }

    // Optional child id, attached in the same call as the mount.
    auto mount(std::shared_ptr<OMDemiurgeNode> child, const std::string &id) -> std::shared_ptr<OMDemiurgeNode>
    {
        auto self = mount(child);
        setChildId(child, id);
        return self;
    }

    auto mountDirect(std::shared_ptr<OMDemiurgeNode> child)
    {
        child->parent = this;
        children.push_back(child);

        YGNodeInsertChild(yogaNode, child->yogaNode, children.size() - 1);
    }

    // Inserts a child before all existing children. Overlay roots use this so
    // they receive pointer events before the normal UI below them.
    auto mountDirect(std::shared_ptr<OMDemiurgeNode> child, const std::string &id)
    {
        mountDirect(child);
        setChildId(child, id);
    }

    auto mountFront(std::shared_ptr<OMDemiurgeNode> child)
    {
        child->parent = this;
        children.insert(children.begin(), child);

        YGNodeInsertChild(yogaNode, child->yogaNode, 0);
    }

    auto mountFront(std::shared_ptr<OMDemiurgeNode> child, const std::string &id)
    {
        mountFront(child);
        setChildId(child, id);
    }

    // Optional per-parent child id table. Direct children can carry one id;
    // lookup works in both directions (node -> id, id -> node). umount removes
    // the tag together with the child.
    auto setChildId(const std::shared_ptr<OMDemiurgeNode> &child, const std::string &id) -> bool;
    void clearChildId(const std::shared_ptr<OMDemiurgeNode> &child);
    auto getChildId(const OMDemiurgeNode *child) const -> std::optional<std::string>;
    auto getChildId(const std::shared_ptr<OMDemiurgeNode> &child) const -> std::optional<std::string>
    {
        return getChildId(child.get());
    }
    auto findChildById(const std::string &id) -> std::shared_ptr<OMDemiurgeNode>;

    // Tooltip settings are stored by OMDemiurgeTooltip, not by the node. The
    // panel is mounted on the UI root and submitted in the overlay layer.
    auto setTooltip(const std::string &text, float delay = 0.6f) -> std::shared_ptr<OMDemiurgeNode>;
    // Cursor tooltip for dense UI such as inventory slots: no hover delay,
    // follows the pointer, and only fades on enter/exit.
    auto setImmediateTooltip(const std::string &text) -> std::shared_ptr<OMDemiurgeNode>;
    auto getTooltip() const -> std::string;
    void setTooltipDelay(float seconds);
    auto getTooltipDelay() const -> float;
    void hideTooltip();
    auto isTooltipHovered() const -> bool;

    auto getRoot() -> std::shared_ptr<OMDemiurgeNode>
    {
        auto cur = shared_from_this();

        while (cur->parent)
        {
            cur = cur->parent->shared_from_this();
        }

        return cur;
    }

    // Structural changes that happen during event dispatch (e.g. mounting an
    // overlay popup) are deferred until the frame can safely traverse children.
    void deferAction(std::function<void()> action)
    {
        std::lock_guard<std::mutex> lock(deferredMutex);
        deferredActions.push_back(std::move(action));
    }

    void flushDeferredActions()
    {
        std::vector<std::function<void()>> actions;
        {
            std::lock_guard<std::mutex> lock(deferredMutex);
            actions = std::move(deferredActions);
            deferredActions.clear();
        }

        for (auto &action : actions)
        {
            if (action)
            {
                action();
            }
        }
    }

    // While a modal child (e.g. an overlay popup) is set on the root, mouse
    // events are dispatched only inside that subtree, so controls underneath
    // cannot capture the pointer or steal the mouse-up.
    void setModalChild(const std::shared_ptr<OMDemiurgeNode> &child)
    {
        std::lock_guard<std::mutex> lock(modalMutex);
        modalStack.erase(std::remove_if(modalStack.begin(), modalStack.end(),
                                        [&](const std::weak_ptr<OMDemiurgeNode> &item) {
                                            auto current = item.lock();
                                            return !current || current == child;
                                        }),
                         modalStack.end());

        if (child)
        {
            modalStack.push_back(child);
        }

        modalChild = child;
    }

    auto hasModalChild() -> bool
    {
        std::lock_guard<std::mutex> lock(modalMutex);
        promoteModalChild();
        return !modalChild.expired();
    }

    void clearModalChild(const std::shared_ptr<OMDemiurgeNode> &child)
    {
        std::lock_guard<std::mutex> lock(modalMutex);
        modalStack.erase(std::remove_if(modalStack.begin(), modalStack.end(),
                                        [&](const std::weak_ptr<OMDemiurgeNode> &item) {
                                            auto current = item.lock();
                                            return !current || current == child;
                                        }),
                         modalStack.end());
        modalChild.reset();
        promoteModalChild();
    }

    void setLayerOverlay(bool value)
    {
        layerOverlay = value;
    }

    auto isLayerOverlay() const -> bool
    {
        return layerOverlay;
    }

    virtual void umount(std::shared_ptr<OMDemiurgeNode> child)
    {
        auto f = std::find(children.begin(), children.end(), child);
        if (f != children.end())
        {
            // A focused/captured control must not keep keyboard/mouse ownership
            // after it leaves the tree.
            releaseFocusWithin(child);

            if (auto captured = fetchMouseCapture())
            {
                for (auto cur = captured.get(); cur != nullptr; cur = cur->parent)
                {
                    if (cur == child.get())
                    {
                        captured->releaseMouse();
                        break;
                    }
                }
            }

            clearChildId(child);
            clearModalChild(child);

            YGNodeRemoveChild(yogaNode, child->yogaNode);
            child->remove();

            child->parent = nullptr;
            children.erase(f);
        }
    }

    void layout(float width, float height);
    inline auto boundary() -> OMDemiurgeRect
    {
        return {YGNodeLayoutGetLeft(yogaNode), YGNodeLayoutGetTop(yogaNode), YGNodeLayoutGetWidth(yogaNode),
                YGNodeLayoutGetHeight(yogaNode)};
    }

    virtual auto submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void = 0;
    virtual auto remove() -> void = 0;
    virtual auto syncLayout() -> void;

    // True when this node manages clipping for its own subtree; ancestor panes
    // stop their clip propagation here.
    virtual auto isClipBoundary() const -> bool
    {
        return false;
    }

    auto fetchChildren() const -> const std::vector<std::shared_ptr<OMDemiurgeNode>> &
    {
        return children;
    }

    // Invisible nodes are skipped by hit-testing and event dispatch. Rendering is
    // managed by the owner (tab pages submit only the active page).
    auto setVisible(bool value) -> void
    {
        visible = value;
    }
    auto isVisible() const -> bool
    {
        return visible;
    }

    auto syncLayoutAll() -> void;
    auto syncBoundary(float x, float y) -> void;

    virtual auto update() -> void
    {
        for (auto ch : children)
        {
            ch->update();
        }
    }

    virtual auto processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
    {
        return Ignored;
    }
    virtual auto processMouseMove(float x, float y) -> OMDemiurgeEventResult
    {
        return Ignored;
    }
    virtual auto processMouseEnter(float x, float y) -> OMDemiurgeEventResult
    {
        return Ignored;
    }
    virtual auto processMouseExit(float x, float y) -> OMDemiurgeEventResult
    {
        return Ignored;
    }
    virtual auto processMouseScroll(float x, float y, float wx, float wy) -> OMDemiurgeEventResult
    {
        return Ignored;
    }
    virtual auto processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult
    {
        return Ignored;
    }
    // Unified event entry: mouse events are routed by layout bounds (with
    // enter/exit synthesis), keyboard/text events are broadcast to the tree.
    void acceptEvent(const OMDemiurgeEvent &event);

    // Mouse capture: while held, MouseMove/MouseUp are delivered to this node even
    // when the pointer leaves its bounds. The capture is released on MouseUp.
    void captureMouse();
    void releaseMouse();
    auto hasMouseCapture() -> bool;
    auto containsPoint(float x, float y) -> bool;

    // True when the point is inside this node and every ancestor is visible and
    // the point is not excluded by an active clip. Used by overlay hit-testing
    // (tooltips) so controls on hidden tab pages or outside a ScrollPane viewport
    // cannot steal hover.
    auto isHoverableAt(float x, float y) -> bool;

    // Clipping: point this node at another node whose layout bounds define the
    // visible range. The clip applies to this node and its whole subtree, and is
    // intersected with the clip ranges of the ancestors.
    void setClipNode(const std::shared_ptr<OMDemiurgeNode> &node)
    {
        clipNodeRef = node;
    }
    auto fetchClipNode() const -> std::shared_ptr<OMDemiurgeNode>
    {
        return clipNodeRef.lock();
    }
    auto resolveClipBound() -> std::optional<OMDemiurgeRect>;

    // Focus: at most one focused node per window tree. Gaining/losing focus walks the
    // whole ancestor chain, so every node between the root and the focused node is
    // notified (and therefore counts as focused while the focus is inside it).
    void setFocusable(bool value);
    auto isFocusable() const -> bool
    {
        return focusable;
    }
    void requestFocus();
    void releaseFocus();

    // Releases focus if the currently focused node is inside the given subtree.
    static auto releaseFocusWithin(const std::shared_ptr<OMDemiurgeNode> &root) -> void;
    auto hasFocus() -> bool;
    static auto fetchFocusedNode() -> std::shared_ptr<OMDemiurgeNode>
    {
        return focusedNode.lock();
    }

    virtual auto processFocusGained() -> void
    {
    }
    virtual auto processFocusLost() -> void
    {
    }

    static auto fetchMouseCapture() -> std::shared_ptr<OMDemiurgeNode>
    {
        return capturedNode.lock();
    }

    // While a descendant holds the mouse capture, ancestors get a chance to
    // take over drags (used by ScrollPane to scroll even when the press started
    // on an interactive control).
    virtual auto processNestedDrag(const std::shared_ptr<OMDemiurgeNode> &captured,
                                   const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
    {
        return Ignored;
    }

    // Dispatch one event to this node only.
    virtual auto processEvent(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
    {
        switch (event.type)
        {
        case OMDemiurgeEventType::MouseDown:
            return processMouseDown(event.x, event.y, event.button);
        case OMDemiurgeEventType::MouseMove:
            return processMouseMove(event.x, event.y);
        case OMDemiurgeEventType::MouseUp:
            return processMouseUp(event.x, event.y, event.button);
        case OMDemiurgeEventType::MouseEnter:
            return processMouseEnter(event.x, event.y);
        case OMDemiurgeEventType::MouseExit:
            return processMouseExit(event.x, event.y);
        case OMDemiurgeEventType::MouseWheel:
            return processMouseScroll(event.x, event.y, event.wheelX, event.wheelY);
        case OMDemiurgeEventType::KeyDown:
            return processKeyDown(event);
        case OMDemiurgeEventType::KeyUp:
            return processKeyUp(event);
        case OMDemiurgeEventType::TextInput:
            return processTextInput(event);
        case OMDemiurgeEventType::TextEditing:
            return processTextEditing(event);
        default:
            return Ignored;
        }
    }

    virtual auto processKeyDown(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
    {
        return Ignored;
    }
    virtual auto processKeyUp(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
    {
        return Ignored;
    }
    virtual auto processTextInput(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
    {
        return Ignored;
    }
    virtual auto processTextEditing(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
    {
        return Ignored;
    }

    // Re-runs hover enter/exit synthesis at the last pointer position. Called
    // after layout when the UI may have moved under a stationary pointer.
    void refreshHover();

    // Runtime class name (demangled), used by the UI tree dump.
    virtual auto typeName() const -> std::string;

    // Human-readable style dump: "{key=value, ...}".
    auto styleDump() const -> std::string;

    // Logs this node and its whole subtree through OMLogger, one line per node.
    auto dumpTree() const -> void;

    // Forces this node to re-run its style-driven submit logic (used after
    // elements were removed and the node becomes visible again).
    inline auto invalidate() -> void
    {
        stylesStorage.invalidate();
    }

    inline auto style(std::string s, std::any a) -> std::shared_ptr<OMDemiurgeNode>
    {
        stylesStorage.put(s, a);
        return shared_from_this();
    }

    // Absolute (screen space) bounds computed by the last layout pass.
    auto fetchLayoutBound() -> OMDemiurgeRect
    {
        return stylesStorage.get<OMDemiurgeRect>("layoutBound");
    }

    // Element opacity, separate from the color's alpha. Applies to the node's
    // own element only; children keep their own opacity.
    auto fetchOpacity() -> float
    {
        return std::clamp(stylesStorage.get<float>("opacity", 1.0f), 0.0f, 1.0f);
    }

    auto fetchBorderInsets() -> OMDemiurgeEdgeInsets
    {
        try
        {
            return stylesStorage.get<OMDemiurgeEdgeInsets>("border");
        }
        catch (const std::bad_any_cast &)
        {
            return OMDemiurgeEdgeInsets{0.0f, 0.0f, 0.0f, 0.0f};
        }
    }

    inline auto style(std::initializer_list<std::pair<std::string, std::any>> d) -> std::shared_ptr<OMDemiurgeNode>
    {
        for (auto &p : d)
        {
            stylesStorage.put(p.first, p.second);
        }
        return shared_from_this();
    }

    inline void bindEventBus(event::OMEventBusWrap &buswrap)
    {
        buswrap.append(event::MouseMotion, [&](event::OMEvent &e) {
            OMDemiurgeEvent ev;
            ev.type = OMDemiurgeEventType::MouseMove;
            ev.x = e.mousemotion.x;
            ev.y = e.mousemotion.y;
            ev.button = e.mousemotion.button;
            acceptEvent(ev);
        });
        buswrap.append(event::MouseUp, [&](event::OMEvent &e) {
            OMDemiurgeEvent ev;
            ev.type = OMDemiurgeEventType::MouseUp;
            ev.x = e.mousebutton.x;
            ev.y = e.mousebutton.y;
            ev.button = e.mousebutton.button;
            acceptEvent(ev);
        });
        buswrap.append(event::MouseDown, [&](event::OMEvent &e) {
            OMDemiurgeEvent ev;
            ev.type = OMDemiurgeEventType::MouseDown;
            ev.x = e.mousebutton.x;
            ev.y = e.mousebutton.y;
            ev.button = e.mousebutton.button;
            acceptEvent(ev);
        });
        buswrap.append(event::MouseWheel, [&](event::OMEvent &e) {
            OMDemiurgeEvent ev;
            ev.type = OMDemiurgeEventType::MouseWheel;
            ev.x = e.mousewheel.x;
            ev.y = e.mousewheel.y;
            ev.wheelX = e.mousewheel.wheelx;
            ev.wheelY = e.mousewheel.wheely;
            acceptEvent(ev);
        });
        buswrap.append(event::KeyDown, [&](event::OMEvent &e) {
            OMDemiurgeEvent ev;
            ev.type = OMDemiurgeEventType::KeyDown;
            ev.keycode = e.key.keycode;
            ev.modifier = e.key.modifier;
            acceptEvent(ev);
        });
        buswrap.append(event::KeyUp, [&](event::OMEvent &e) {
            OMDemiurgeEvent ev;
            ev.type = OMDemiurgeEventType::KeyUp;
            ev.keycode = e.key.keycode;
            ev.modifier = e.key.modifier;
            acceptEvent(ev);
        });
        buswrap.append(event::TextInput, [&](event::OMEvent &e) {
            OMDemiurgeEvent ev;
            ev.type = OMDemiurgeEventType::TextInput;
            ev.text = e.text;
            acceptEvent(ev);
        });
        buswrap.append(event::TextEditing, [&](event::OMEvent &e) {
            OMDemiurgeEvent ev;
            ev.type = OMDemiurgeEventType::TextEditing;
            ev.text = e.text;
            ev.compositionStart = e.selectionStart;
            ev.compositionLength = e.selectionLength;
            acceptEvent(ev);
        });
    }

  protected:
    auto dispatchCapturedEvent(const OMDemiurgeEvent &event) -> void;

    float mx = INFINITY, my = INFINITY;
    bool hoverInside = false;
    OMDemiurgeStyles stylesStorage;
    YGNodeRef yogaNode;
    OMDemiurgeNode *parent = nullptr;
    std::vector<std::shared_ptr<OMDemiurgeNode>> children;

    mutable std::mutex childIdMutex;
    std::unordered_map<const OMDemiurgeNode *, std::string> childIdByNode;
    std::unordered_map<std::string, std::weak_ptr<OMDemiurgeNode>> childById;

    bool focusable = false;
    bool visible = true;
    std::weak_ptr<OMDemiurgeNode> clipNodeRef;
    std::vector<std::function<void()>> deferredActions;
    std::mutex deferredMutex;
    std::weak_ptr<OMDemiurgeNode> modalChild;
    std::vector<std::weak_ptr<OMDemiurgeNode>> modalStack;
    std::mutex modalMutex;

    // Caller must hold modalMutex.
    void promoteModalChild()
    {
        modalChild.reset();

        while (!modalStack.empty())
        {
            auto current = modalStack.back().lock();

            if (current)
            {
                modalChild = current;
                return;
            }

            modalStack.pop_back();
        }
    }

    bool layerOverlay = false;

    inline static std::weak_ptr<OMDemiurgeNode> capturedNode;
    inline static std::weak_ptr<OMDemiurgeNode> capturedRoot;
    inline static std::weak_ptr<OMDemiurgeNode> focusedNode;
    inline static std::weak_ptr<OMDemiurgeNode> focusedRoot;
};
} // namespace openminecraft::renderer::common::demiurge

#endif
