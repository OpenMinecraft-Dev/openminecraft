#ifndef OM_ELYSIA_THREADMODEL_HPP
#define OM_ELYSIA_THREADMODEL_HPP

#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
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
    void *flag = nullptr;
    OMElysiaJavaFrame *caller;
};

class OMElysiaThread
{
  public:
    bool threadInited = false;
    uintptr_t stackStart = 0;
    uintptr_t stackEnd = 0;

    OMElysiaJNIEnv interface;
    struct
    {
        uint8_t *pc = nullptr;
        uintptr_t stackPointer = 0;
        OMElysiaJavaFrame *frame = nullptr;
    } zero;

    std::function<void()> cleaner = []() {};

    OMElysiaThread()
    {
    }

    void registerThread()
    {
        std::lock_guard lg(mapMutex);

        threadMap[std::this_thread::get_id()] = this;
    }

    ~OMElysiaThread()
    {
        std::lock_guard lg(mapMutex);
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
