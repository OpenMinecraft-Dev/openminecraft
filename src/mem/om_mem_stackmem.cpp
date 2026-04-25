#include "openminecraft/mem/om_mem_stackmem.hpp"
#include <cstddef>
#include <cstdint>
#include <pthread.h>

namespace openminecraft::mem::stack
{
uintptr_t fetchStackBase(std::thread thr)
{
    pthread_attr_t attr;
    pthread_getattr_np(thr.native_handle(), &attr);

    void *stackTop;
    size_t stackLength;
    pthread_attr_getstack(&attr, &stackTop, &stackLength);
    pthread_attr_destroy(&attr);

    return reinterpret_cast<uintptr_t>(stackTop) + stackLength;
}
uintptr_t fetchStackTop(std::thread thr)
{
    pthread_attr_t attr;
    pthread_getattr_np(thr.native_handle(), &attr);

    void *stackTop;
    size_t stackLength;
    pthread_attr_getstack(&attr, &stackTop, &stackLength);
    pthread_attr_destroy(&attr);

    return reinterpret_cast<uintptr_t>(stackTop);
}
} // namespace openminecraft::mem::stack
