#ifndef OM_DEMIURGE_NODE_HPP
#define OM_DEMIURGE_NODE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_styles.hpp"
#include "yoga/YGNode.h"
#include "yoga/YGNodeLayout.h"
#include "yoga/YGNodeStyle.h"
#include <algorithm>
#include <any>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <yoga/Yoga.h>

namespace openminecraft::renderer::common::demiurge
{
class OMDemiurgeRendererHandler;
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

class OMDemiurgeNode : public std::enable_shared_from_this<OMDemiurgeNode>
{
  public:
    OMDemiurgeNode();
    ~OMDemiurgeNode();

    virtual void mount(std::shared_ptr<OMDemiurgeNode> child)
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
