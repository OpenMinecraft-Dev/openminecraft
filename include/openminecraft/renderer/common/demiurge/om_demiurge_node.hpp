#ifndef OM_DEMIURGE_NODE_HPP
#define OM_DEMIURGE_NODE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_event.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_styles.hpp"
#include "openminecraft/renderer/common/event/om_eventbus_wrap.hpp"
#include "yoga/YGNode.h"
#include "yoga/YGNodeLayout.h"
#include <algorithm>
#include <any>
#include <cmath>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <yoga/Yoga.h>
#include <cstdint>
#include <array>

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

    auto mountDirect(std::shared_ptr<OMDemiurgeNode> child)
    {
        child->parent = this;
        children.push_back(child);

        YGNodeInsertChild(yogaNode, child->yogaNode, children.size() - 1);
    }

    virtual void umount(std::shared_ptr<OMDemiurgeNode> child)
    {
        auto f = std::find(children.begin(), children.end(), child);
        if (f != children.end())
        {
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

    virtual auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void = 0;
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
    virtual auto processNestedDrag(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
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
    OMDemiurgeStyles stylesStorage;
    YGNodeRef yogaNode;
    OMDemiurgeNode *parent = nullptr;
    std::vector<std::shared_ptr<OMDemiurgeNode>> children;

    bool focusable = false;
    bool visible = true;
    std::weak_ptr<OMDemiurgeNode> clipNodeRef;

    inline static std::weak_ptr<OMDemiurgeNode> capturedNode;
    inline static std::weak_ptr<OMDemiurgeNode> capturedRoot;
    inline static std::weak_ptr<OMDemiurgeNode> focusedNode;
    inline static std::weak_ptr<OMDemiurgeNode> focusedRoot;
};
} // namespace openminecraft::renderer::common::demiurge

#endif
