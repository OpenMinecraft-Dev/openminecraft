#ifndef OM_ANIMATION_MULTITIMELINE_HPP
#define OM_ANIMATION_MULTITIMELINE_HPP

#include "openminecraft/renderer/common/animation/om_animation_timeline_value.hpp"
#include <utility>
#include <vector>
namespace openminecraft::renderer::common::animation
{
enum OMAnimationBlend
{
    Add,
    Multiply
};

template <int L, typename T> class OMAnimationMultiTimeline
{
  public:
    OMAnimationMultiTimeline() = default;

    auto timeline(OMAnimationTimelineValue<L, T> t, OMAnimationBlend bl) -> OMAnimationMultiTimeline<L, T> &
    {
        timelines.push_back({t, bl});
        return *this;
    }

    auto operator[](int i) -> T
    {
        T data = {};

        for (auto &p : timelines)
        {
            T d = p.first[i];
            switch (p.second)
            {
            case Add:
                data += d;
                break;
            case Multiply:
            default:
                data *= d;
                break;
            }
        }

        return data;
    }

  private:
    std::vector<std::pair<OMAnimationTimelineValue<L, T>, OMAnimationBlend>> timelines;
};
} // namespace openminecraft::renderer::common::animation

#endif