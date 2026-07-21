#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "yoga/YGNode.h"
#include "yoga/YGNodeStyle.h"

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
OMDemiurgeNode::OMDemiurgeNode()
{
    yogaNode = YGNodeNew();
    YGNodeStyleSetFlexDirection(yogaNode, YGFlexDirectionColumn);
    YGNodeStyleSetWidthPercent(yogaNode, 100);
    YGNodeStyleSetHeightPercent(yogaNode, 100);
}
OMDemiurgeNode::~OMDemiurgeNode()
{
    YGNodeFree(yogaNode);
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
        // 交叉轴水平 → alignSelf
        YGNodeStyleSetAlignSelf(node, horiz);
        // 主轴垂直 → margin auto
        if (vert == YGAlignCenter)
        {
            YGNodeStyleSetMarginAuto(node, YGEdgeTop);
            YGNodeStyleSetMarginAuto(node, YGEdgeBottom);
        }
        else if (vert == YGAlignFlexEnd)
        {
            YGNodeStyleSetMarginAuto(node, YGEdgeTop);
            YGNodeStyleSetMargin(node, YGEdgeBottom, 0); // 清除 auto
        }
        else
        {
            YGNodeStyleSetMargin(node, YGEdgeTop, 0);
            YGNodeStyleSetMargin(node, YGEdgeBottom, 0);
        }
    }
    else
    {
        // 父容器为 Row：主轴水平 → margin auto，交叉轴垂直 → alignSelf
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

void OMDemiurgeNode::syncStyle()
{
    setSizeToYoga(yogaNode, style.width, true);
    setSizeToYoga(yogaNode, style.height, false);
    YGNodeStyleSetMinWidth(yogaNode, style.minWidth);
    YGNodeStyleSetMinHeight(yogaNode, style.minHeight);
    YGNodeStyleSetMaxWidth(yogaNode, style.maxWidth);
    YGNodeStyleSetMaxHeight(yogaNode, style.maxHeight);

#define ygsync(ygname, myname, ygside, myside) YGNodeStyleSet##ygname(yogaNode, YGEdge##ygside, style.myname.myside);
    ygsync(Padding, padding, Top, top);
    ygsync(Padding, padding, Bottom, bottom);
    ygsync(Padding, padding, Left, left);
    ygsync(Padding, padding, Right, right);
    ygsync(Margin, margin, Top, top);
    ygsync(Margin, margin, Bottom, bottom);
    ygsync(Margin, margin, Left, left);
    ygsync(Margin, margin, Right, right);
    ygsync(Border, border, Top, top);
    ygsync(Border, border, Bottom, bottom);
    ygsync(Border, border, Left, left);
    ygsync(Border, border, Right, right);

    YGNodeStyleSetPositionType(yogaNode, style.position == Absolute ? YGPositionTypeAbsolute : YGPositionTypeRelative);

    applyAlignment(yogaNode, style.alignment);
}
void OMDemiurgeNode::layout(float width, float height)
{
    YGNodeCalculateLayout(yogaNode, width, height, YGDirectionLTR);
}
} // namespace openminecraft::renderer::common::demiurge
