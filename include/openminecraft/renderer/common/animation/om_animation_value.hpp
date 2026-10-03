#ifndef OM_ANIMATION_VALUE_HPP
#define OM_ANIMATION_VALUE_HPP

#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include <chrono>
namespace openminecraft::renderer::common::animation
{
template <typename T> class OMAnimationValue
{
  public:
    OMAnimationValue(T value) : value(value)
    {
    }

    void animateTo(T value, OMEasingFunc func, float sec)
    {
        this->sec = sec;
        beginTimepoint = std::chrono::steady_clock::now();
        this->easingFunc = func;
        beginValue = this->value;
        targetValue = value;
        status = 0.0f;
    }

    void setTo(T value)
    {
        this->value = value;
        beginValue = value;
        targetValue = value;
        status = -1.0f;
    }

    auto get() -> T
    {
        if (status >= 0.0f)
        {
            float elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - beginTimepoint)
                    .count() /
                1000.0f;
            status = sec > 0.0f ? elapsed / sec : 1.0f;
            if (status > 1.0f)
            {
                status = -1.0f;
                value = targetValue;
                return value;
            }
            else
            {
                float i = easingFunc(status);
                value = beginValue * (1 - i) + targetValue * i;
            }
        }

        return value;
    }

  private:
    OMEasingFunc easingFunc = [](float x) { return x; };
    std::chrono::steady_clock::time_point beginTimepoint;
    float sec = 0.0f;
    T beginValue;
    T targetValue;
    float status = -1.0f;

    T value;
};
} // namespace openminecraft::renderer::common::animation

#endif