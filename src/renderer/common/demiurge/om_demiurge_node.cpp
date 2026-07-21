#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
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
}
OMDemiurgeNode::~OMDemiurgeNode()
{
    YGNodeFree(yogaNode);
}
void OMDemiurgeNode::syncStyle()
{
    YGNodeStyleSetMinWidth(yogaNode, style.minWidth);
    YGNodeStyleSetMinHeight(yogaNode, style.minHeight);
    YGNodeStyleSetMaxWidth(yogaNode, style.maxWidth);
    YGNodeStyleSetMaxHeight(yogaNode, style.maxHeight);
}
void OMDemiurgeNode::layout()
{
}
} // namespace openminecraft::renderer::common::demiurge
