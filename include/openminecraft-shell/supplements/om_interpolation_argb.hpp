#ifndef OM_INTERPOLATION_ARGB_HPP
#define OM_INTERPOLATION_ARGB_HPP

#include "openminecraft/renderer/common/animation/om_animation_interpolation.hpp"
#include <functional>

namespace openminecraft::shell::supplements
{
const static std::function<int(int, int, float)> argbInterpolate = [](int colorA, int colorB, float ratio) {
    int aAlpha = (colorA >> 24) & 0xff;
    int aRed = (colorA >> 16) & 0xff;
    int aGreen = (colorA >> 8) & 0xff;
    int aBlue = (colorA >> 0) & 0xff;
    int bAlpha = (colorB >> 24) & 0xff;
    int bRed = (colorB >> 16) & 0xff;
    int bGreen = (colorB >> 8) & 0xff;
    int bBlue = (colorB >> 0) & 0xff;

    using openminecraft::renderer::common::animation::normalInterpolation;

    int outAlpha = normalInterpolation<int, float>(aAlpha, bAlpha, ratio);
    int outRed = normalInterpolation<int, float>(aRed, bRed, ratio);
    int outGreen = normalInterpolation<int, float>(aGreen, bGreen, ratio);
    int outBlue = normalInterpolation<int, float>(aBlue, bBlue, ratio);

    return outAlpha << 24 | outRed << 16 | outGreen << 8 | outBlue;
};
} // namespace openminecraft::shell::supplements

#endif