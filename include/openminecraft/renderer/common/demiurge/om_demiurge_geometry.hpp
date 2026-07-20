#ifndef OM_DEMIURGE_GEOMETRY_HPP
#define OM_DEMIURGE_GEOMETRY_HPP

namespace openminecraft::renderer::common::demiurge
{
struct OMDemiurgeSize
{
    enum Unit
    {
        Pixel,
        Percent,
        Fit
    } unit = Fit;

    float value = 0;

    inline auto pixels(float px) -> OMDemiurgeSize
    {
        return {Pixel, px};
    }

    inline auto percent(float pct) -> OMDemiurgeSize
    {
        return {Percent, pct};
    }

    inline auto fit() -> OMDemiurgeSize
    {
        return {Fit, 0};
    }

    inline auto fill() -> OMDemiurgeSize
    {
        return {Percent, 1.0f};
    }
};
} // namespace openminecraft::renderer::common::demiurge

#endif
