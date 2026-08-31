#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "yoga/YGNode.h"
#include "yoga/YGNodeStyle.h"
#include <any>
#include <array>
#include <iostream>

namespace openminecraft::renderer::common::demiurge
{
static auto toYGDirection(OMDemiurgeDirection d) -> YGFlexDirection
{
    switch (d)
    {
    case Row:
    default:
        return YGFlexDirectionRow;
    case RowReverse:
        return YGFlexDirectionRowReverse;
    case Column:
        return YGFlexDirectionColumn;
    case ColumnReverse:
        return YGFlexDirectionColumnReverse;
    }
}

static auto toYGWrap(OMDemiurgeWrap w) -> YGWrap
{
    switch (w)
    {
    case Wrap:
        return YGWrapWrap;
    case WrapReverse:
        return YGWrapWrapReverse;
    default:
    case NoWrap:
        return YGWrapNoWrap;
    }
}

static auto toYGJustify(OMDemiurgeAlign a) -> YGJustify
{
    switch (a)
    {
    default:
    case FlexStart:
        return YGJustifyFlexStart;
    case Auto:
    case Stretch:
    case Baseline:
    case Center:
        return YGJustifyCenter;
    case FlexEnd:
        return YGJustifyFlexEnd;
    case SpaceAround:
        return YGJustifySpaceAround;
    case SpaceBetween:
        return YGJustifySpaceBetween;
    case SpaceEvenly:
        return YGJustifySpaceEvenly;
    }
}
static auto toYGAlign(OMDemiurgeAlign a) -> YGAlign
{
    switch (a)
    {
    default:
    case Auto:
        return YGAlignAuto;
    case FlexStart:
        return YGAlignFlexStart;
    case Center:
        return YGAlignCenter;
    case FlexEnd:
        return YGAlignFlexEnd;
    case Stretch:
        return YGAlignStretch;
    case Baseline:
        return YGAlignBaseline;
    case SpaceBetween:
        return YGAlignSpaceBetween;
    case SpaceAround:
        return YGAlignSpaceAround;
    case SpaceEvenly:
        return YGAlignSpaceEvenly;
    }
}

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
}
OMDemiurgeNode::~OMDemiurgeNode()
{
    YGNodeFree(yogaNode);
}

auto OMDemiurgeNode::syncLayout() -> void
{
    if (stylesStorage.isModified())
    {
        using namespace openminecraft::binary::hash;
        for (auto &p : stylesStorage)
        {
            switch (hash_compile_time(p.first.c_str()))
            {
            case "minWidth"_hash:
                YGNodeStyleSetMinWidth(yogaNode, std::any_cast<float>(p.second));
                break;
            case "minHeight"_hash:
                YGNodeStyleSetMinHeight(yogaNode, std::any_cast<float>(p.second));
                break;
            case "maxWidth"_hash:
                YGNodeStyleSetMaxWidth(yogaNode, std::any_cast<float>(p.second));
                break;
            case "maxHeight"_hash:
                YGNodeStyleSetMaxHeight(yogaNode, std::any_cast<float>(p.second));
                break;
            case "width"_hash:
                setSizeToYoga(yogaNode, std::any_cast<OMDemiurgeSize>(p.second), true);
                break;
            case "height"_hash:
                setSizeToYoga(yogaNode, std::any_cast<OMDemiurgeSize>(p.second), false);
                break;
            case "alignItems"_hash:
                YGNodeStyleSetAlignItems(yogaNode, toYGAlign(std::any_cast<OMDemiurgeAlign>(p.second)));
                break;
            case "alignContent"_hash:
                YGNodeStyleSetAlignContent(yogaNode, toYGAlign(std::any_cast<OMDemiurgeAlign>(p.second)));
                break;
            case "justifyContent"_hash:
                YGNodeStyleSetJustifyContent(yogaNode, toYGJustify(std::any_cast<OMDemiurgeAlign>(p.second)));
                break;
            case "alignSelf"_hash:
                YGNodeStyleSetAlignSelf(yogaNode, toYGAlign(std::any_cast<OMDemiurgeAlign>(p.second)));
                break;
            case "flexBasis"_hash: {
                auto s = std::any_cast<OMDemiurgeSize>(p.second);
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
                break;
            }
            case "flexGrow"_hash:
                YGNodeStyleSetFlexGrow(yogaNode, std::any_cast<float>(p.second));
                break;
            case "flexShrink"_hash:
                YGNodeStyleSetFlexShrink(yogaNode, std::any_cast<float>(p.second));
                break;
            case "flexGap"_hash: {
                auto g = std::any_cast<OMDemiurgeSize>(p.second);
                if (g.unit == OMDemiurgeSize::Percent)
                {
                    YGNodeStyleSetGapPercent(yogaNode, YGGutterAll, g.value * 100.0f);
                }
                else
                {
                    YGNodeStyleSetGap(yogaNode, YGGutterAll, g.value);
                }
                break;
            }
            case "offsetY"_hash:
            case "offsetX"_hash: {
                auto edge = p.first == "offsetX" ? YGEdgeLeft : YGEdgeTop;
                auto s = std::any_cast<OMDemiurgeSize>(p.second);
                switch (s.unit)
                {
                case OMDemiurgeSize::Pixel:
                    YGNodeStyleSetPosition(yogaNode, edge, s.value);
                    break;
                case OMDemiurgeSize::Percent:
                    YGNodeStyleSetPositionPercent(yogaNode, edge, s.value * 100.0f);
                    break;
                case OMDemiurgeSize::Fit:
                    YGNodeStyleSetPositionAuto(yogaNode, edge);
                    break;
                }
                break;
            }
            case "position"_hash: {
                YGNodeStyleSetPositionType(yogaNode, std::any_cast<OMDemiurgePosition>(p.second) == Absolute
                                                         ? YGPositionTypeAbsolute
                                                         : YGPositionTypeRelative);
                break;
            }
            case "flexDirection"_hash: {
                YGNodeStyleSetFlexDirection(yogaNode, toYGDirection(std::any_cast<OMDemiurgeDirection>(p.second)));
                break;
            }
            case "flexWrap"_hash: {
                YGNodeStyleSetFlexWrap(yogaNode, toYGWrap(std::any_cast<OMDemiurgeWrap>(p.second)));
                break;
            }
            case "margin"_hash: {
                auto ii = std::any_cast<std::array<OMDemiurgeSize, 4>>(p.second);
                auto &top = ii[0];
                auto &bottom = ii[1];
                auto &left = ii[2];
                auto &right = ii[3];

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
                break;
            }
            case "border"_hash: {
                auto insets = std::any_cast<OMDemiurgeEdgeInsets>(p.second);
                YGNodeStyleSetBorder(yogaNode, YGEdgeTop, insets.top);
                YGNodeStyleSetBorder(yogaNode, YGEdgeBottom, insets.bottom);
                YGNodeStyleSetBorder(yogaNode, YGEdgeLeft, insets.left);
                YGNodeStyleSetBorder(yogaNode, YGEdgeRight, insets.right);
                break;
            }
            case "padding"_hash: {
                auto insets = std::any_cast<OMDemiurgeEdgeInsets>(p.second);
                YGNodeStyleSetPadding(yogaNode, YGEdgeTop, insets.top);
                YGNodeStyleSetPadding(yogaNode, YGEdgeBottom, insets.bottom);
                YGNodeStyleSetPadding(yogaNode, YGEdgeLeft, insets.left);
                YGNodeStyleSetPadding(yogaNode, YGEdgeRight, insets.right);
                break;
            }
            }
        }
    }
}

auto OMDemiurgeNode::syncLayoutAll() -> void
{
    syncLayout();
    for (auto f : children)
    {
        f->syncLayoutAll();
    }
}

void OMDemiurgeNode::layout(float width, float height)
{
    syncLayoutAll();
    YGNodeCalculateLayout(yogaNode, width, height, YGDirectionLTR);
    syncBoundary(0, 0);
}

auto OMDemiurgeNode::syncBoundary(float x, float y) -> void
{
    auto b = boundary();
    b.x += x;
    b.y += y;
    try
    {
        auto bo = stylesStorage.get<OMDemiurgeRect>("layoutBound");
        if (b.x == bo.x && b.y == bo.y && b.width == bo.width && b.height == bo.height)
        {
            goto updateChildren;
        }
    }
    catch (std::bad_any_cast &e)
    {
    }

    stylesStorage.put("layoutBound", b);

updateChildren:
    for (auto f : children)
    {
        f->syncBoundary(b.x, b.y);
    }
}

void OMDemiurgeNode::acceptEvent(float x, float y)
{
    try
    {
        auto bo = stylesStorage.get<OMDemiurgeRect>("layoutBound");

        if (x >= bo.x && x <= bo.x + bo.width && y >= bo.y && y <= bo.y + bo.height)
        {
            std::cout << typeid(*this).name() << std::endl;

            for (auto f : children)
            {
                f->acceptEvent(x, y);
            }
        }
    }
    catch (std::bad_any_cast &e)
    {
    }
}
} // namespace openminecraft::renderer::common::demiurge
