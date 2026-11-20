#ifndef OM_ANIMATION_INTERPOLATION_HPP
#define OM_ANIMATION_INTERPOLATION_HPP

namespace openminecraft::renderer::common::animation
{
template <typename T, typename A> static auto normalInterpolation(T a, T b, A alpha) -> T
{
    return a + (b - a) * alpha;
}
} // namespace openminecraft::renderer::common::animation

#endif