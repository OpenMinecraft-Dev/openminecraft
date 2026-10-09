#ifndef OM_INTERPOLATION_ARGB_HPP
#define OM_INTERPOLATION_ARGB_HPP

#include "openminecraft/renderer/common/animation/om_animation_interpolation.hpp"
#include <functional>

namespace openminecraftshell::supplements
{
const static std::function<int(int, int, float)> argbInterpolate = [](int ca, int cb, float p) {
    int aa = (ca >> 24) & 0xff;
    int ar = (ca >> 16) & 0xff;
    int ag = (ca >> 8) & 0xff;
    int ab = (ca >> 0) & 0xff;
    int ba = (cb >> 24) & 0xff;
    int br = (cb >> 16) & 0xff;
    int bg = (cb >> 8) & 0xff;
    int bb = (cb >> 0) & 0xff;

    using openminecraft::renderer::common::animation::normalInterpolation;

    int a = normalInterpolation<int, float>(aa, ba, p);
    int r = normalInterpolation<int, float>(ar, br, p);
    int g = normalInterpolation<int, float>(ag, bg, p);
    int b = normalInterpolation<int, float>(ab, bb, p);

    return a << 24 | r << 16 | g << 8 | b;
};
} // namespace openminecraftshell::supplements

#endif