#ifndef OM_DEMIURGE_NODE_HPP
#define OM_DEMIURGE_NODE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "yoga/YGNode.h"
#include "yoga/YGNodeLayout.h"
#include "yoga/YGNodeStyle.h"
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

static void applyAlignment(YGNodeRef node, OMDemiurgeAlignment alignment,
                           YGFlexDirection parentDirection = YGFlexDirectionColumn)
{
    YGAlign horiz = YGAlignFlexStart;
    YGAlign vert = YGAlignFlexStart;

    switch (alignment)
    {
    case TopLeft:
        horiz = YGAlignFlexStart;
        vert = YGAlignFlexStart;
        break;
    case TopCenter:
        horiz = YGAlignCenter;
        vert = YGAlignFlexStart;
        break;
    case TopRight:
        horiz = YGAlignFlexEnd;
        vert = YGAlignFlexStart;
        break;
    case CenterLeft:
        horiz = YGAlignFlexStart;
        vert = YGAlignCenter;
        break;
    case Center:
        horiz = YGAlignCenter;
        vert = YGAlignCenter;
        break;
    case CenterRight:
        horiz = YGAlignFlexEnd;
        vert = YGAlignCenter;
        break;
    case BottomLeft:
        horiz = YGAlignFlexStart;
        vert = YGAlignFlexEnd;
        break;
    case BottomCenter:
        horiz = YGAlignCenter;
        vert = YGAlignFlexEnd;
        break;
    case BottomRight:
        horiz = YGAlignFlexEnd;
        vert = YGAlignFlexEnd;
        break;
    }

    bool isColumn = (parentDirection == YGFlexDirectionColumn || parentDirection == YGFlexDirectionColumnReverse);

    if (isColumn)
    {
        YGNodeStyleSetAlignSelf(node, horiz);
        if (vert == YGAlignCenter)
        {
            YGNodeStyleSetMarginAuto(node, YGEdgeTop);
            YGNodeStyleSetMarginAuto(node, YGEdgeBottom);
        }
        else if (vert == YGAlignFlexEnd)
        {
            YGNodeStyleSetMarginAuto(node, YGEdgeTop);
            YGNodeStyleSetMargin(node, YGEdgeBottom, 0);
        }
        else
        {
            YGNodeStyleSetMargin(node, YGEdgeTop, 0);
            YGNodeStyleSetMargin(node, YGEdgeBottom, 0);
        }
    }
    else
    {
        YGNodeStyleSetAlignSelf(node, vert);
        if (horiz == YGAlignCenter)
        {
            YGNodeStyleSetMarginAuto(node, YGEdgeLeft);
            YGNodeStyleSetMarginAuto(node, YGEdgeRight);
        }
        else if (horiz == YGAlignFlexEnd)
        {
            YGNodeStyleSetMarginAuto(node, YGEdgeLeft);
            YGNodeStyleSetMargin(node, YGEdgeRight, 0);
        }
        else
        {
            YGNodeStyleSetMargin(node, YGEdgeLeft, 0);
            YGNodeStyleSetMargin(node, YGEdgeRight, 0);
        }
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

        OMDemiurgeDirection flexDirection;
        OMDemiurgeWrap flexWrap;
    } style;
    inline auto flexWrap(OMDemiurgeWrap w) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.flexWrap = w;
        YGNodeStyleSetFlexWrap(yogaNode, w == Wrap ? YGWrapWrap : YGWrapNoWrap);

        return shared_from_this();
    }
    inline auto flexDirection(OMDemiurgeDirection d) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.flexDirection = d;
        YGNodeStyleSetFlexDirection(yogaNode, d == Row ? YGFlexDirectionRow : YGFlexDirectionColumn);

        return shared_from_this();
    }
    inline auto position(OMDemiurgePosition p) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.position = p;
        YGNodeStyleSetPositionType(yogaNode, p == Absolute ? YGPositionTypeAbsolute : YGPositionTypeRelative);

        return shared_from_this();
    }
    inline auto alignment(OMDemiurgeAlignment a) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.alignment = a;
        applyAlignment(yogaNode, a);

        return shared_from_this();
    }
    inline auto offset(float x, float y) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.offsetx = x;
        style.offsety = y;
        YGNodeStyleSetPosition(yogaNode, YGEdgeTop, x);
        YGNodeStyleSetPosition(yogaNode, YGEdgeLeft, y);

        return shared_from_this();
    }
    inline auto border(OMDemiurgeEdgeInsets insets) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.border = insets;
        YGNodeStyleSetBorder(yogaNode, YGEdgeTop, insets.top);
        YGNodeStyleSetBorder(yogaNode, YGEdgeBottom, insets.bottom);
        YGNodeStyleSetBorder(yogaNode, YGEdgeLeft, insets.left);
        YGNodeStyleSetBorder(yogaNode, YGEdgeRight, insets.right);

        return shared_from_this();
    }
    inline auto padding(OMDemiurgeEdgeInsets insets) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.margin = insets;
        YGNodeStyleSetPadding(yogaNode, YGEdgeTop, insets.top);
        YGNodeStyleSetPadding(yogaNode, YGEdgeBottom, insets.bottom);
        YGNodeStyleSetPadding(yogaNode, YGEdgeLeft, insets.left);
        YGNodeStyleSetPadding(yogaNode, YGEdgeRight, insets.right);

        return shared_from_this();
    }
    inline auto margin(OMDemiurgeEdgeInsets insets) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.margin = insets;
        YGNodeStyleSetMargin(yogaNode, YGEdgeTop, insets.top);
        YGNodeStyleSetMargin(yogaNode, YGEdgeBottom, insets.bottom);
        YGNodeStyleSetMargin(yogaNode, YGEdgeLeft, insets.left);
        YGNodeStyleSetMargin(yogaNode, YGEdgeRight, insets.right);

        return shared_from_this();
    }
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
    inline auto minWidth(float w) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.minWidth = w;
        YGNodeStyleSetMinWidth(yogaNode, w);

        return shared_from_this();
    }
    inline auto minHeight(float w) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.minHeight = w;
        YGNodeStyleSetMinHeight(yogaNode, w);

        return shared_from_this();
    }
    inline auto maxWidth(float w) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.maxWidth = w;
        YGNodeStyleSetMaxWidth(yogaNode, w);

        return shared_from_this();
    }
    inline auto maxHeight(float w) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.maxHeight = w;
        YGNodeStyleSetMaxHeight(yogaNode, w);

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
