#ifndef OM_ELYSIA_THREADMODEL_HPP
#define OM_ELYSIA_THREADMODEL_HPP

#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include <atomic>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
namespace openminecraft::vm::elysia
{
class OMElysiaThread;
extern std::map<std::thread::id, OMElysiaThread *> threadMap;
extern std::mutex mapMutex;

struct OMElysiaJavaFrame
{
    OMElysiaMethod *method;
    uint8_t *returnAddr;
    OMElysiaJavaFrame *caller;
};

class OMElysiaThread
{
  public:
    bool threadInited = false;
    void *stackStart;
    void *stackEnd;
    struct
    {
        uint8_t *pc = nullptr;
        void *stackPointer = nullptr;
        OMElysiaJavaFrame *frame = nullptr;
    } zero;

    std::function<void()> cleaner = []() {};

    OMElysiaThread()
    {
        std::lock_guard lg(mapMutex);

        threadMap[std::this_thread::get_id()] = this;
    }
    ~OMElysiaThread()
    {
        cleaner();
        threadMap.erase(std::this_thread::get_id());
    }
};

class OMElysiaThreadMetadata
{
  public:
    OMElysiaThread *metadata;

    OMElysiaThreadMetadata()
    {
        metadata = new OMElysiaThread;
    }

    ~OMElysiaThreadMetadata()
    {
        delete metadata;
    }
};

extern thread_local OMElysiaThreadMetadata thisThread;
} // namespace openminecraft::vm::elysia

#endif
