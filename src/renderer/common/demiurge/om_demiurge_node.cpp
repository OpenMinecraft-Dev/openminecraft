#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
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
            YGNodeStyleSetWidthPercent(node, size.value);
        else
            YGNodeStyleSetHeightPercent(node, size.value);
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
    ygsync(Margin, margin, Top, top);
    ygsync(Border, border, Top, top);
}
void OMDemiurgeNode::layout()
{
}
} // namespace openminecraft::renderer::common::demiurge
