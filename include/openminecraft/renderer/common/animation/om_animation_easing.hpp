#ifndef OM_ANIMATION_EASING_HPP
#define OM_ANIMATION_EASING_HPP

#include <cmath>
#include <functional>

namespace openminecraft::renderer::common::animation
{
constexpr float PI = 3.14159265358979;
using OMEasingFuncSignature = float(float);
using OMEasingFunc = std::function<OMEasingFuncSignature>;

template <typename T> auto easeInSine(T x) -> T
{
    return 1 - cos((x * PI) / 2);
}

template <typename T> auto easeOutSine(T x) -> T
{
    return sin((x * PI) / 2);
}

template <typename T> auto easeInOutSine(T x) -> T
{
    return -(cos(PI * x) - 1) / 2;
}

template <typename T> auto easeInQuad(T x) -> T
{
    return x * x;
}

template <typename T> auto easeOutQuad(T x) -> T
{
    return 1 - (1 - x) * (1 - x);
}

template <typename T> auto easeInOutQuad(T x) -> T
{
    return x < 0.5 ? 2 * x * x : 1 - pow(-2 * x + 2, 2) / 2;
}

template <typename T> auto easeInCubic(T x) -> T
{
    return x * x * x;
}

template <typename T> auto easeOutCubic(T x) -> T
{
    return 1 - (1 - x) * (1 - x) * (1 - x);
}

template <typename T> auto easeInOutCubic(T x) -> T
{
    return x < 0.5 ? 4 * x * x * x : 1 - pow(-2 * x + 2, 3) / 2;
}

template <typename T> auto easeInQuart(T x) -> T
{
    return x * x * x * x;
}

template <typename T> auto easeOutQuart(T x) -> T
{
    return 1 - (1 - x) * (1 - x) * (1 - x) * (1 - x);
}

template <typename T> auto easeInOutQuart(T x) -> T
{
    return x < 0.5 ? 8 * x * x * x * x : 1 - pow(-2 * x + 2, 4) / 2;
}

template <typename T> auto easeInQuint(T x) -> T
{
    return x * x * x * x * x;
}

template <typename T> auto easeOutQuint(T x) -> T
{
    return 1 - (1 - x) * (1 - x) * (1 - x) * (1 - x) * (1 - x);
}

template <typename T> auto easeInOutQuint(T x) -> T
{
    return x < 0.5 ? 16 * x * x * x * x * x : 1 - pow(-2 * x + 2, 5) / 2;
}

template <typename T> auto easeInExpo(T x) -> T
{
    return x == 0 ? 0 : pow(2, 10 * x - 10);
}

template <typename T> auto easeOutExpo(T x) -> T
{
    return x == 1 ? 1 : 1 - pow(2, -10 * x);
}

template <typename T> auto easeInOutExpo(T x) -> T
{
    return x == 0 ? 0 : x == 1 ? 1 : x < 0.5 ? pow(2, 20 * x - 10) / 2 : (2 - pow(2, -20 * x + 10)) / 2;
}

template <typename T> auto easeInCirc(T x) -> T
{
    return 1 - sqrt(1 - pow(x, 2));
}

template <typename T> auto easeOutCirc(T x) -> T
{
    return sqrt(1 - pow(x - 1, 2));
}

template <typename T> auto easeInOutCirc(T x) -> T
{
    return x < 0.5 ? (1 - sqrt(1 - pow(2 * x, 2))) / 2 : (sqrt(1 - pow(-2 * x + 2, 2)) + 1) / 2;
}

template <typename T> auto easeInBack(T x) -> T
{
    return 2.70158 * x * x * x - 1.70158 * x * x;
}

template <typename T> auto easeOutBack(T x) -> T
{
    return 1 + 2.70158 * pow(x - 1, 3) + 1.70158 * pow(x - 1, 2);
}

template <typename T> auto easeInOutBack(T x) -> T
{
    const float c1 = 1.70158;
    const float c2 = c1 * 1.525;
    return x < 0.5 ? (pow(2 * x, 2) * ((c2 + 1) * 2 * x - c2)) / 2
                   : (pow(2 * x - 2, 2) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;
}

template <typename T> auto easeInElastic(T x) -> T
{
    const auto c4 = (2 * PI) / 3;

    return x == 0 ? 0 : x == 1 ? 1 : -pow(2, 10 * x - 10) * sin((x * 10 - 10.75) * c4);
}

template <typename T> auto easeOutElastic(T x) -> T
{
    const auto c4 = (2 * PI) / 3;

    return x == 0 ? 0 : x == 1 ? 1 : pow(2, -10 * x) * sin((x * 10 - 0.75) * c4) + 1;
}

template <typename T> auto easeInOutElastic(T x) -> T
{
    const auto c5 = (2 * PI) / 4.5;

    return x == 0    ? 0
           : x == 1  ? 1
           : x < 0.5 ? -(pow(2, 20 * x - 10) * sin((20 * x - 11.125) * c5)) / 2
                     : (pow(2, -20 * x + 10) * sin((20 * x - 11.125) * c5)) / 2 + 1;
}

template <typename T> auto easeOutBounce(T x) -> T
{
    const auto n1 = 7.5625;
    const auto d1 = 2.75;

    if (x < 1 / d1)
    {
        return n1 * x * x;
    }
    else if (x < 2 / d1)
    {
        return n1 * (x -= 1.5 / d1) * x + 0.75;
    }
    else if (x < 2.5 / d1)
    {
        return n1 * (x -= 2.25 / d1) * x + 0.9375;
    }
    else
    {
        return n1 * (x -= 2.625 / d1) * x + 0.984375;
    }
}

template <typename T> auto easeInBounce(T x) -> T
{
    return 1 - easeOutBounce(1 - x);
}

template <typename T> auto easeInOutBounce(T x) -> T
{
    return x < 0.5 ? (1 - easeOutBounce(1 - 2 * x)) / 2 : (1 + easeOutBounce(2 * x - 1)) / 2;
}

template <typename T> auto easeBezier(T x, T p1x, T p1y, T p2x, T p2y) -> T
{
    if (x <= T(0))
        return T(0);
    if (x >= T(1))
        return T(1);

    auto xd = static_cast<double>(x);
    auto p1xd = static_cast<double>(p1x);
    auto p1yd = static_cast<double>(p1y);
    auto p2xd = static_cast<double>(p2x);
    auto p2yd = static_cast<double>(p2y);

    auto bezierX = [&](double t) -> double {
        return 3.0 * (1.0 - t) * (1.0 - t) * t * p1xd + 3.0 * (1.0 - t) * t * t * p2xd + t * t * t;
    };
    auto bezierY = [&](double t) -> double {
        return 3.0 * (1.0 - t) * (1.0 - t) * t * p1yd + 3.0 * (1.0 - t) * t * t * p2yd + t * t * t;
    };

    double low = 0.0, high = 1.0;
    double t = 0.5;
    for (int i = 0; i < 50; ++i)
    {
        t = (low + high) * 0.5;
        double xval = bezierX(t);
        if (std::abs(xval - xd) < 1e-7)
            break;
        if (xval < xd)
            low = t;
        else
            high = t;
    }

    double y = bezierY(t);
    return static_cast<T>(y);
}

template <typename T> auto symmetricEaseBezier(T x, T px, T py) -> T
{
    return easeBezier(x, px, py, 1.0f - px, 1.0f - py);
}

} // namespace openminecraft::renderer::common::animation

#endif