#ifndef OM_DEMIURGE_NODE_HPP
#define OM_DEMIURGE_NODE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "yoga/YGNode.h"
#include "yoga/YGNodeLayout.h"
#include <algorithm>
#include <memory>
#include <vector>
#include <yoga/Yoga.h>

namespace openminecraft::renderer::common::demiurge
{
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
    struct Style
    {
        OMDemiurgeSize width{OMDemiurgeSize::fit()};
        OMDemiurgeSize height{OMDemiurgeSize::fit()};
        float minWidth = 0;
        float maxWidth = 1e308;
        float minHeight = 0;
        float maxHeight = 1e308;

        OMDemiurgeEdgeInsets margin = {}, padding = {}, border = {};
        OMDemiurgePosition position = Relative;
        OMDemiurgeAlignment alignment = TopLeft;

        float offsetx = 0;
        float offsety = 0;
    } style;

    inline auto width(OMDemiurgeSize size) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.width = size;
        setSizeToYoga(yogaNode, size, true);
        return shared_from_this();
    }
    inline auto height(OMDemiurgeSize size) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.height = size;
        setSizeToYoga(yogaNode, size, false);
        return shared_from_this();
    }

    OMDemiurgeNode();
    ~OMDemiurgeNode();

    inline void mount(std::shared_ptr<OMDemiurgeNode> child)
    {
        child->parent = this;
        children.push_back(child);

        YGNodeInsertChild(yogaNode, child->yogaNode, children.size() - 1);
    }

    inline void umount(std::shared_ptr<OMDemiurgeNode> child)
    {
        auto f = std::find(children.begin(), children.end(), child);
        if (f != children.end())
        {
            YGNodeRemoveChild(yogaNode, child->yogaNode);
            child->parent = nullptr;
            children.erase(f);
        }
    }

    void syncStyle();
    void layout(float width, float height);
    inline auto boundary() -> OMDemiurgeRect
    {
        return {YGNodeLayoutGetLeft(yogaNode), YGNodeLayoutGetTop(yogaNode), YGNodeLayoutGetWidth(yogaNode),
                YGNodeLayoutGetHeight(yogaNode)};
    }

  private:
    YGNodeRef yogaNode;
    OMDemiurgeNode *parent = nullptr;
    std::vector<std::shared_ptr<OMDemiurgeNode>> children;
};
} // namespace openminecraft::renderer::common::demiurge

#endif
