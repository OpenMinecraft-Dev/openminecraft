#ifndef OM_DEMIURGE_NODE_HPP
#define OM_DEMIURGE_NODE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_styles.hpp"
#include "yoga/YGNode.h"
#include "yoga/YGNodeLayout.h"
#include <algorithm>
#include <any>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <yoga/Yoga.h>
#include <cstdint>

namespace openminecraft::renderer::common::demiurge
{
class OMDemiurgeRendererHandler;
enum OMDemiurgeEventType
{
    MouseMove,
    MouseDown,
    MouseUp,
    MouseWheel,
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

    virtual auto processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
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

  protected:
    OMDemiurgeStyles stylesStorage;
    YGNodeRef yogaNode;
    OMDemiurgeNode *parent = nullptr;
    std::vector<std::shared_ptr<OMDemiurgeNode>> children;
};
} // namespace openminecraft::renderer::common::demiurge

#endif
