#ifndef OM_DEMIURGE_NODE_HPP
#define OM_DEMIURGE_NODE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_styles.hpp"
#include "openminecraft/renderer/common/event/om_eventbus.hpp"
#include "yoga/YGNode.h"
#include "yoga/YGNodeLayout.h"
#include <algorithm>
#include <any>
#include <cmath>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <yoga/Yoga.h>
#include <cstdint>
#include <array>

namespace openminecraft::renderer::common::demiurge
{
class OMDemiurgeRendererHandler;
enum OMDemiurgeEventType
{
    MouseMove,
    MouseDown,
    MouseUp,
    MouseWheel,
    MouseEnter,
    MouseExit,
    KeyDown,
    KeyUp,
};
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
    void acceptEvent(float x, float y, OMDemiurgeEventType type, uint8_t, void * = nullptr);

    inline auto style(std::string s, std::any a) -> std::shared_ptr<OMDemiurgeNode>
    {
        stylesStorage.put(s, a);
        return shared_from_this();
    }

    inline auto style(std::initializer_list<std::pair<std::string, std::any>> d) -> std::shared_ptr<OMDemiurgeNode>
    {
        for (auto &p : d)
        {
            stylesStorage.put(p.first, p.second);
        }
        return shared_from_this();
    }

    inline void bindEventBus(event::OMEventBusSDL &bus)
    {
        bus.append(SDL_EVENT_MOUSE_MOTION, [&](SDL_Event &e) -> void {
            acceptEvent(e.button.x, e.button.y, demiurge::MouseMove, e.button.button);
        });
        bus.append(SDL_EVENT_MOUSE_BUTTON_UP, [&](SDL_Event &e) -> void {
            acceptEvent(e.button.x, e.button.y, demiurge::MouseUp, e.button.button);
        });
        bus.append(SDL_EVENT_MOUSE_BUTTON_DOWN, [&](SDL_Event &e) -> void {
            acceptEvent(e.button.x, e.button.y, demiurge::MouseDown, e.button.button);
        });
        bus.append(SDL_EVENT_MOUSE_WHEEL, [&](SDL_Event &e) -> void {
            acceptEvent(e.wheel.mouse_x, e.wheel.mouse_y, demiurge::MouseWheel, e.button.button,
                        std::array<float, 2>{e.wheel.x, e.wheel.y}.data());
        });
        bus.append(SDL_EVENT_KEY_DOWN,
                   [&](SDL_Event &e) -> void { acceptEvent(INFINITY, INFINITY, demiurge::KeyDown, e.key.key); });
        bus.append(SDL_EVENT_KEY_UP,
                   [&](SDL_Event &e) -> void { acceptEvent(INFINITY, INFINITY, demiurge::KeyUp, e.key.key); });

        bus.append(SDL_EVENT_FINGER_MOTION, [&](SDL_Event &e) {
            int w, h;
            SDL_GetWindowSize(SDL_GetWindowFromEvent(&e), &w, &h);
            acceptEvent(e.tfinger.x * w, e.tfinger.y * h, demiurge::MouseMove, e.tfinger.fingerID);
        });
        bus.append(SDL_EVENT_FINGER_UP, [&](SDL_Event &e) {
            int w, h;
            SDL_GetWindowSize(SDL_GetWindowFromEvent(&e), &w, &h);
            acceptEvent(e.tfinger.x * w, e.tfinger.y * h, demiurge::MouseUp, e.tfinger.fingerID);
        });
        bus.append(SDL_EVENT_FINGER_DOWN, [&](SDL_Event &e) {
            int w, h;
            SDL_GetWindowSize(SDL_GetWindowFromEvent(&e), &w, &h);
            acceptEvent(e.tfinger.x * w, e.tfinger.y * h, demiurge::MouseDown, e.tfinger.fingerID);
        });
    }

  protected:
    float mx = INFINITY, my = INFINITY;
    OMDemiurgeStyles stylesStorage;
    YGNodeRef yogaNode;
    OMDemiurgeNode *parent = nullptr;
    std::vector<std::shared_ptr<OMDemiurgeNode>> children;
};
} // namespace openminecraft::renderer::common::demiurge

#endif
