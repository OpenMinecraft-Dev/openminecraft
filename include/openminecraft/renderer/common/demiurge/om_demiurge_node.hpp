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
class OMDemiurgeNode
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

        auto fixed() -> bool
        {
            return width.unit == OMDemiurgeSize::Pixel && height.unit == OMDemiurgeSize::Pixel;
        }
    } style;

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
