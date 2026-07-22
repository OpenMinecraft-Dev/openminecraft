#ifndef OM_DEMIURGE_NODE_HPP
#define OM_DEMIURGE_NODE_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "yoga/YGNode.h"
#include "yoga/YGNodeLayout.h"
#include "yoga/YGNodeStyle.h"
#include <algorithm>
#include <any>
#include <memory>
#include <string>
#include <unordered_map>
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
    struct Style
    {
        OMDemiurgeSize width{OMDemiurgeSize::fit()};
        OMDemiurgeSize height{OMDemiurgeSize::fit()};
        float minWidth = 0;
        float maxWidth = 1e308;
        float minHeight = 0;
        float maxHeight = 1e308;

        OMDemiurgeSize marginTop = {}, marginBottom = {}, marginLeft = {}, marginRight = {};

        OMDemiurgeEdgeInsets padding = {}, border = {};
        OMDemiurgePosition position = Relative;

        float offsetx = 0;
        float offsety = 0;

        OMDemiurgeDirection flexDirection;
        OMDemiurgeWrap flexWrap;

        float flexGrow;
        float flexShrink;

        OMDemiurgeAlign alignItems;
        OMDemiurgeAlign alignContent;
        OMDemiurgeAlign justifyContent;
        OMDemiurgeSize flexGap;
        OMDemiurgeSize flexBasis;

        OMDemiurgeAlign alignSelf;
    } style;
    inline auto alignItems(OMDemiurgeAlign a) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.alignItems = a;
        YGNodeStyleSetAlignItems(yogaNode, toYGAlign(a));
        return shared_from_this();
    }
    inline auto alignContent(OMDemiurgeAlign a) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.alignContent = a;
        YGNodeStyleSetAlignContent(yogaNode, toYGAlign(a));
        return shared_from_this();
    }
    inline auto justifyContent(OMDemiurgeAlign a) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.justifyContent = a;
        YGNodeStyleSetJustifyContent(yogaNode, toYGJustify(a));
        return shared_from_this();
    }
    inline auto alignSelf(OMDemiurgeAlign a) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.alignSelf = a;
        YGNodeStyleSetAlignSelf(yogaNode, toYGAlign(a));
        return shared_from_this();
    }
    inline auto flexBasis(OMDemiurgeSize s) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.flexBasis = s;
        switch (s.unit)
        {
        case OMDemiurgeSize::Pixel:
            YGNodeStyleSetFlexBasis(yogaNode, s.value);
            break;
        case OMDemiurgeSize::Percent:
            YGNodeStyleSetFlexBasisPercent(yogaNode, s.value * 100.0f);
            break;
        default:
            YGNodeStyleSetFlexBasisAuto(yogaNode);
            break;
        }
        return shared_from_this();
    }
    inline auto flexRatio(float grow, float shrink) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.flexGrow = grow;
        style.flexShrink = shrink;
        YGNodeStyleSetFlexGrow(yogaNode, grow);
        YGNodeStyleSetFlexShrink(yogaNode, shrink);
        return shared_from_this();
    }
    inline auto flexGap(OMDemiurgeSize g) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.flexGap = g;
        if (g.unit == OMDemiurgeSize::Percent)
        {
            YGNodeStyleSetGapPercent(yogaNode, YGGutterAll, g.value * 100.0f);
        }
        else
        {
            YGNodeStyleSetGap(yogaNode, YGGutterAll, g.value);
        }
        return shared_from_this();
    }
    inline auto flexWrap(OMDemiurgeWrap w) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.flexWrap = w;
        YGNodeStyleSetFlexWrap(yogaNode, toYGWrap(w));
        return shared_from_this();
    }
    inline auto flexDirection(OMDemiurgeDirection d) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.flexDirection = d;
        YGNodeStyleSetFlexDirection(yogaNode, toYGDirection(d));
        return shared_from_this();
    }
    inline auto position(OMDemiurgePosition p) -> std::shared_ptr<OMDemiurgeNode>
    {
        style.position = p;
        YGNodeStyleSetPositionType(yogaNode, p == Absolute ? YGPositionTypeAbsolute : YGPositionTypeRelative);

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
        style.padding = insets;
        YGNodeStyleSetPadding(yogaNode, YGEdgeTop, insets.top);
        YGNodeStyleSetPadding(yogaNode, YGEdgeBottom, insets.bottom);
        YGNodeStyleSetPadding(yogaNode, YGEdgeLeft, insets.left);
        YGNodeStyleSetPadding(yogaNode, YGEdgeRight, insets.right);

        return shared_from_this();
    }
    inline auto margin(OMDemiurgeSize top, OMDemiurgeSize bottom, OMDemiurgeSize left, OMDemiurgeSize right)
        -> std::shared_ptr<OMDemiurgeNode>
    {
        style.marginTop = top;
        style.marginBottom = bottom;
        style.marginLeft = left;
        style.marginRight = right;

#define update(source, type)                                                                                           \
    switch (source.unit)                                                                                               \
    {                                                                                                                  \
    case OMDemiurgeSize::Pixel:                                                                                        \
        YGNodeStyleSetMargin(yogaNode, YGEdge##type, source.value);                                                    \
        break;                                                                                                         \
    case OMDemiurgeSize::Percent:                                                                                      \
        YGNodeStyleSetMarginPercent(yogaNode, YGEdge##type, source.value * 100.0f);                                    \
        break;                                                                                                         \
    default:                                                                                                           \
        YGNodeStyleSetMarginAuto(yogaNode, YGEdge##type);                                                              \
        break;                                                                                                         \
    }

        update(top, Top);
        update(bottom, Bottom);
        update(left, Left);
        update(right, Right);

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

    virtual auto render(OMRendererTask *, OMDemiurgeRendererHandler *, float depth) -> void = 0;

    inline auto setStyle(std::string s, std::any a) -> std::shared_ptr<OMDemiurgeNode>
    {
        styles[s] = a;
        return shared_from_this();
    }

  protected:
    std::unordered_map<std::string, std::any> styles;
    YGNodeRef yogaNode;
    OMDemiurgeNode *parent = nullptr;
    std::vector<std::shared_ptr<OMDemiurgeNode>> children;
};
} // namespace openminecraft::renderer::common::demiurge

#endif
