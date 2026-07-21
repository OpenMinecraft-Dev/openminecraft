#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "yoga/YGNode.h"
#include "yoga/YGNodeStyle.h"

namespace openminecraft::renderer::common::demiurge
{
OMDemiurgeNode::OMDemiurgeNode()
{
    yogaNode = YGNodeNew();
    YGNodeStyleSetFlexDirection(yogaNode, YGFlexDirectionColumn);
    YGNodeStyleSetWidthPercent(yogaNode, 100);
    YGNodeStyleSetHeightPercent(yogaNode, 100);

    syncStyle();
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

    YGNodeStyleSetPosition(yogaNode, YGEdgeTop, style.offsety);
    YGNodeStyleSetPosition(yogaNode, YGEdgeLeft, style.offsetx);

    YGNodeStyleSetFlexDirection(yogaNode, YGFlexDirectionRow);
    YGNodeStyleSetFlexWrap(yogaNode, YGWrapWrap);
}
void OMDemiurgeNode::layout(float width, float height)
{
    YGNodeCalculateLayout(yogaNode, width, height, YGDirectionLTR);
}
} // namespace openminecraft::renderer::common::demiurge
