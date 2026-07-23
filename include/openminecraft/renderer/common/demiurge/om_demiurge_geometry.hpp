#ifndef OM_DEMIURGE_GEOMETRY_HPP
#define OM_DEMIURGE_GEOMETRY_HPP

#include "yoga/Yoga.h"

namespace openminecraft::renderer::common::demiurge
{
struct OMDemiurgeRect
{
    float x, y, width, height;
};
struct OMDemiurgeEdgeInsets
{
    float top, bottom, left, right;
};
enum OMDemiurgePosition
{
    Relative,
    Absolute
};
enum OMDemiurgeDirection
{
    Row,
    RowReverse,
    Column,
    ColumnReverse
};
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
enum OMDemiurgeWrap
{
    Wrap,
    WrapReverse,
    NoWrap
};
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
enum OMDemiurgeAlign
{
    Auto,
    FlexStart,
    Center,
    FlexEnd,
    Stretch,
    Baseline,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};
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
struct OMDemiurgeSize
{
    enum Unit
    {
        Pixel,
        Percent,
        Fit
    } unit = Fit;

    float value = 0;

    constexpr inline static auto pixels(float px) -> OMDemiurgeSize
    {
        return {Pixel, px};
    }

    constexpr inline static auto percent(float pct) -> OMDemiurgeSize
    {
        return {Percent, pct};
    }

    constexpr inline static auto fit() -> OMDemiurgeSize
    {
        return {Fit, 0};
    }

    constexpr inline static auto fill() -> OMDemiurgeSize
    {
        return {Percent, 1.0f};
    }
};
constexpr auto operator""_percent(unsigned long long s) -> OMDemiurgeSize
{
    return OMDemiurgeSize::percent(static_cast<float>(s) / 100.0f);
}
constexpr auto operator""_px(unsigned long long s) -> OMDemiurgeSize
{
    return OMDemiurgeSize::pixels(static_cast<float>(s));
}
} // namespace openminecraft::renderer::common::demiurge

#endif
