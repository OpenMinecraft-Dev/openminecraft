#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "yoga/YGNode.h"
#include "yoga/YGNodeStyle.h"

namespace openminecraft::renderer::common::demiurge
{
OMDemiurgeNode::OMDemiurgeNode()
{
    yogaNode = YGNodeNew();
    YGNodeStyleSetAlignContent(yogaNode, YGAlignStretch);
    YGNodeStyleSetJustifyContent(yogaNode, YGJustifySpaceAround);
}
OMDemiurgeNode::~OMDemiurgeNode()
{
    YGNodeFree(yogaNode);
}

void OMDemiurgeNode::layout(float width, float height)
{
    YGNodeCalculateLayout(yogaNode, width, height, YGDirectionLTR);
}
} // namespace openminecraft::renderer::common::demiurge
