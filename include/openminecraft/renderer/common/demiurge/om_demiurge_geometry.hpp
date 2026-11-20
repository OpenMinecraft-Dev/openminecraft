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

enum OMDemiurgeWrap
{
    Wrap,
    WrapReverse,
    NoWrap
};

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
