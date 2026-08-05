#ifndef OM_EVENTBUS_HPP
#define OM_EVENTBUS_HPP

#include "SDL3/SDL_events.h"
#include <functional>
#include <unordered_map>
#include <vector>
namespace openminecraft::renderer::common::event
{
template <typename T, typename S> class OMEventBus
{
  public:
    OMEventBus() = default;
    ~OMEventBus() = default;

    inline void append(T t, std::function<void(S &)> h)
    {
        handlers[t].emplace_back(h);
    }

    inline void handle(T t, S &s)
    {
        for (auto m : handlers[t])
        {
            m(s);
        }
    }

    std::unordered_map<T, std::vector<std::function<void(S &)>>> handlers;
};
using OMEventBusSDL = OMEventBus<SDL_EventType, SDL_Event>;
} // namespace openminecraft::renderer::common::event

#endif
