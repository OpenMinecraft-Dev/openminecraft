#ifndef OM_UTIL_TICKER_HPP
#define OM_UTIL_TICKER_HPP

#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <utility>
#include <vector>

namespace openminecraft::util
{
template <typename E> class OMTicker
{
  public:
    OMTicker()
    {
    }
    ~OMTicker() = default;

    void recordEvent(E event)
    {
        eventMap[event] = std::chrono::high_resolution_clock::now();
    }

    void tickStart()
    {
        begin = std::chrono::high_resolution_clock::now();
    }

    template <typename D> std::vector<std::pair<E, uint64_t>> fetchEvents()
    {
        std::vector<std::pair<E, uint64_t>> target;
        for (auto &p : eventMap)
        {
            target.push_back(std::make_pair(p.first, std::chrono::duration_cast<D>(p.second - begin).count()));
        }

        std::sort(target.begin(), target.end(),
                  [](std::pair<E, uint64_t> &p1, std::pair<E, uint64_t> &p2) { return p1.second < p2.second; });

        return target;
    }

    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> begin;
    std::unordered_map<E, std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>> eventMap;
};
} // namespace openminecraft::util

#endif
