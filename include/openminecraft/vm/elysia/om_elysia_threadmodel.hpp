#ifndef OM_ELYSIA_THREADMODEL_HPP
#define OM_ELYSIA_THREADMODEL_HPP

#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include <functional>
#include <thread>
#include <unordered_map>
namespace openminecraft::vm::elysia
{
class OMElysiaThread;
extern std::unordered_map<std::thread::id, OMElysiaThread *> threadMap;

struct OMElysiaJavaFrame
{
    OMElysiaMethod *method;
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
        void *pc = nullptr;
        void *stackPointer = nullptr;
        OMElysiaJavaFrame *frame = nullptr;
    } zero;

    std::function<void()> cleaner = []() {};

    OMElysiaThread()
    {
        threadMap[std::this_thread::get_id()] = this;
    }
    ~OMElysiaThread()
    {
        cleaner();
        threadMap.erase(std::this_thread::get_id());
    }
};

extern thread_local OMElysiaThread threadContext;
} // namespace openminecraft::vm::elysia

#endif
