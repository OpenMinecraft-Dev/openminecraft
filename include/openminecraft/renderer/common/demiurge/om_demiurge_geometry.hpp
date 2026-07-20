#ifndef OM_DEMIURGE_GEOMETRY_HPP
#define OM_DEMIURGE_GEOMETRY_HPP

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
struct OMDemiurgeSize
{
    enum Unit
    {
        Pixel,
        Percent,
        Fit
    } unit = Fit;

    float value = 0;

    inline static auto pixels(float px) -> OMDemiurgeSize
    {
        return {Pixel, px};
    }

    inline static auto percent(float pct) -> OMDemiurgeSize
    {
        return {Percent, pct};
    }

    inline static auto fit() -> OMDemiurgeSize
    {
        return {Fit, 0};
    }

    inline static auto fill() -> OMDemiurgeSize
    {
        return {Percent, 1.0f};
    }
};
} // namespace openminecraft::renderer::common::demiurge

#endif
