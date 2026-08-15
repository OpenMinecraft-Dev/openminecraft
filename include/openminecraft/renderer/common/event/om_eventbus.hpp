#ifndef OM_EVENTBUS_HPP
#define OM_EVENTBUS_HPP

#include "SDL3/SDL_events.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
namespace openminecraft::renderer::common::event
{
template <typename T, typename S> class OMEventBus
{
  public:
    OMEventBus()
    {
        handlerThread = new std::thread([&]() -> auto {
            std::unique_lock<std::mutex> lock(mtx);
            while (active)
            {
                newEvents.wait(lock, [&]() -> auto { return !eventQueue.empty() || !active; });

                if (eventQueue.empty())
                {
                    continue;
                }

                auto &e = eventQueue.front();
                for (auto h : handlers[e.first])
                {
                    h(e.second);
                }

                eventQueue.pop();
            }
        });

        loopThread = new std::thread([&]() -> auto {
            while (active)
            {
                for (auto h : generalHandlers)
                {
                    h();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    ~OMEventBus()
    {
        active = false;
        newEvents.notify_all();
        handlerThread->join();
        delete handlerThread;
        loopThread->join();
        delete loopThread;
    }

    inline void append(T t, std::function<void(S &)> h)
    {
        handlers[t].emplace_back(h);
    }

    inline void appendGeneral(std::function<void()> h)
    {
        generalHandlers.emplace_back(h);
    }

    inline void handle(T t, S &s)
    {
        eventQueue.push({t, s});
        newEvents.notify_one();
    }

    std::unordered_map<T, std::vector<std::function<void(S &)>>> handlers;
    std::vector<std::function<void()>> generalHandlers;

  private:
    std::thread *handlerThread;
    std::thread *loopThread;
    std::atomic_bool active = true;
    std::condition_variable newEvents;
    std::mutex mtx;

    std::queue<std::pair<T, S>> eventQueue;
};
using OMEventBusSDL = OMEventBus<SDL_EventType, SDL_Event>;
} // namespace openminecraft::renderer::common::event

#endif
