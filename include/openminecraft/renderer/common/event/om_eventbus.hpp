#ifndef OM_EVENTBUS_HPP
#define OM_EVENTBUS_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
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
            handlerThread = new std::thread([this]() -> void {
                std::unique_lock<std::mutex> lock(queueMtx);
                while (active)
                {
                    newEvents.wait(lock, [this]() -> bool {
                        return !eventQueue.empty() || !active;
                    });

                    if (!active && eventQueue.empty())
                    {
                        break;
                    }

                    if (eventQueue.empty())
                    {
                        continue;
                    }

                    // 在锁内取走事件，锁外执行回调，避免回调重入 handle() 死锁
                    auto e = std::move(eventQueue.front());
                    eventQueue.pop();
                    lock.unlock();

                    std::vector<std::function<void(S &)>> hs;
                    {
                        std::lock_guard<std::mutex> hlock(handlersMtx);
                        auto it = handlers.find(e.first);
                        if (it != handlers.end())
                        {
                            hs = it->second;
                        }
                    }

                    for (auto &h : hs)
                    {
                        h(e.second);
                    }

                    lock.lock();
                }
            });

            loopThread = new std::thread([this]() -> void {
                while (active)
                {
                    std::vector<std::function<void()>> hs;
                    {
                        std::lock_guard<std::mutex> hlock(generalHandlersMtx);
                        hs = generalHandlers;
                    }

                    for (auto &h : hs)
                    {
                        h();
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            });
        }

        ~OMEventBus()
        {
            {
                std::lock_guard<std::mutex> lock(queueMtx);
                active = false;
            }
            newEvents.notify_all();

            if (handlerThread != nullptr)
            {
                if (handlerThread->joinable())
                {
                    handlerThread->join();
                }
                delete handlerThread;
                handlerThread = nullptr;
            }

            if (loopThread != nullptr)
            {
                if (loopThread->joinable())
                {
                    loopThread->join();
                }
                delete loopThread;
                loopThread = nullptr;
            }
        }

        OMEventBus(const OMEventBus &) = delete;
        OMEventBus &operator=(const OMEventBus &) = delete;

        inline void append(T t, std::function<void(S &)> h)
        {
            std::lock_guard<std::mutex> lock(handlersMtx);
            handlers[t].emplace_back(std::move(h));
        }

        inline void appendGeneral(std::function<void()> h)
        {
            std::lock_guard<std::mutex> lock(generalHandlersMtx);
            generalHandlers.emplace_back(std::move(h));
        }

        inline void handle(T t, S &s)
        {
            {
                std::lock_guard<std::mutex> lock(queueMtx);
                eventQueue.push({std::move(t), s});
            }
            newEvents.notify_one();
        }

    private:
        std::unordered_map<T, std::vector<std::function<void(S &)>>> handlers;
        std::vector<std::function<void()>> generalHandlers;

        std::thread *handlerThread = nullptr;
        std::thread *loopThread = nullptr;
        std::atomic_bool active = true;

        std::condition_variable newEvents;
        std::mutex queueMtx;
        std::mutex handlersMtx;
        std::mutex generalHandlersMtx;

        std::queue<std::pair<T, S>> eventQueue;
    };

} // namespace openminecraft::renderer::common::event

#endif
