#ifndef OM_ANIMATION_TIMELINE_HPP
#define OM_ANIMATION_TIMELINE_HPP

#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/animation/om_animation_interpolation.hpp"
#include <functional>
#include <vector>
namespace openminecraft::renderer::common::animation
{
template <int L, typename T> class OMAnimationTimelineValue
{
  public:
    OMAnimationTimelineValue() = default;

    auto operator[](int t) -> T
    {
        if (keyframes.empty())
        {
            return T{};
        }
        if (keyframes.size() == 1)
        {
            return keyframes[0].second;
        }

        int time = t % L;
        if (time < 0)
            time += L;

        int i = -1;
        for (int j = 0; j < (int)keyframes.size(); ++j)
        {
            if (keyframes[j].first <= time)
            {
                i = j;
            }
            else
            {
                break;
            }
        }

        if (i != -1 && keyframes[i].first == time)
        {
            return keyframes[i].second;
        }

        int start_idx, end_idx;
        int start_tick, end_tick;
        int adjusted_time = time;

        if (i == -1)
        {
            start_idx = (int)keyframes.size() - 1;
            end_idx = 0;
            start_tick = keyframes[start_idx].first - L;
            end_tick = keyframes[end_idx].first;
        }
        else
        {
            start_idx = i;
            end_idx = (i + 1) % keyframes.size();
            start_tick = keyframes[start_idx].first;
            if (end_idx > start_idx)
            {
                end_tick = keyframes[end_idx].first;
            }
            else
            {
                end_tick = keyframes[end_idx].first + L;
            }
            if (adjusted_time < start_tick)
            {
                adjusted_time += L;
            }
        }

        float alpha = 0.0f;
        if (end_tick != start_tick)
        {
            alpha = static_cast<float>(adjusted_time - start_tick) / static_cast<float>(end_tick - start_tick);
        }
        alpha = easingFunc(alpha);

        const T &start_value = keyframes[start_idx].second;
        const T &end_value = keyframes[end_idx].second;
        return interpolator(start_value, end_value, alpha);
    }

    auto append(int t, T v) -> OMAnimationTimelineValue<L, T> &
    {
        keyframes.push_back({t, v});
        return *this;
    }

    auto easing(OMEasingFunc func) -> OMAnimationTimelineValue<L, T> &
    {
        easingFunc = func;
        return *this;
    }

    auto interpolate(std::function<T(T, T, float)> i) -> OMAnimationTimelineValue<L, T> &
    {
        interpolator = i;
        return *this;
    }

  private:
    OMEasingFunc easingFunc = linear<float>;
    std::function<T(T, T, float)> interpolator = normalInterpolation<T, float>;
    std::vector<std::pair<int, T>> keyframes;
};
} // namespace openminecraft::renderer::common::animation

#endif