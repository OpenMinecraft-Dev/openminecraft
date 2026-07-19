#ifndef OM_UTIL_TICKER_HPP
#define OM_UTIL_TICKER_HPP

#include <algorithm>
#include <chrono>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace openminecraft::util
{
struct OMTickerEvent
{
    std::string id;
    bool pop;
};
class OMTicker
{
  public:
    OMTicker() = default;
    ~OMTicker() = default;

    inline void begin()
    {
        ticks.clear();
        while (!items.empty())
        {
            items.pop();
        }
    }

    void push(std::string id)
    {
        items.push(id);
        ticks.emplace_back(OMTickerEvent{id, false}, std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                         std::chrono::steady_clock::now().time_since_epoch())
                                                         .count());
    }
    void pop()
    {
        ticks.emplace_back(OMTickerEvent{items.top(), true}, std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                                 std::chrono::steady_clock::now().time_since_epoch())
                                                                 .count());
        items.pop();
    }

    std::stack<std::string> items;
    std::vector<std::pair<OMTickerEvent, uint64_t>> ticks;
};
} // namespace openminecraft::util

#endif
