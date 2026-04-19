#ifndef OM_ELYSIA_EXECUTOR_ZERO_HPP
#define OM_ELYSIA_EXECUTOR_ZERO_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/om_elysia_field.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <cstdint>
namespace openminecraft::vm::elysia::executor
{
uintptr_t zeroStackAlloc(uint64_t len);
uintptr_t zeroStackPop(uint64_t len);
template <typename T> static void zeroStackPush(T data)
{
    auto d = reinterpret_cast<uintptr_t *>(zeroStackAlloc(sizeof(void *)));
    *d = (uintptr_t)data;
}

template <typename T> static void zeroStackPushW(T data)
{
    if constexpr (sizeof(void *) == 8)
    {
        auto d = reinterpret_cast<uintptr_t *>(zeroStackAlloc(sizeof(void *)));
        *d = *reinterpret_cast<uint64_t *>(&data);
        zeroStackAlloc(sizeof(
            void *)); // gino: wide data need 2 slots, on 64 bit this means one actual slot and another padding slot
    }
    else
    {
        uint64_t d = *reinterpret_cast<uint64_t *>(&data);
        auto dhigh = reinterpret_cast<uintptr_t *>(zeroStackAlloc(sizeof(void *)));
        *dhigh = static_cast<uint32_t>(d >> 32);
        auto dlow = reinterpret_cast<uintptr_t *>(zeroStackAlloc(sizeof(void *)));
        *dlow = static_cast<uint32_t>(d & 0xffffffff);
    }
}

template <typename T> static T zeroStackPeekGet()
{
    return *reinterpret_cast<T *>(thisThread.metadata->zero.stackPointer);
}

template <typename T> static T zeroStackPopGet()
{
    return *reinterpret_cast<T *>(zeroStackPop(sizeof(void *)));
}

template <typename T> static T zeroStackPopWGet()
{
    if constexpr (sizeof(void *) == 8)
    {
        zeroStackPop(sizeof(void *)); // geopeila: padding slot
        return *reinterpret_cast<T *>(zeroStackPop(sizeof(void *)));
    }
    else
    {
        auto dlow = *reinterpret_cast<uint32_t *>(zeroStackPop(sizeof(void *)));
        auto dhigh = *reinterpret_cast<uint32_t *>(zeroStackPop(sizeof(void *)));

        auto d = static_cast<uint64_t>(dhigh) << 32 | dlow;
        return *reinterpret_cast<T *>(&d);
    }
}

void zeroStackPopToStatic(OMElysiaField *field);
void zeroStackPopToField(OMElysiaField *field, OMElysiaOopManager *oop);
uint16_t zeroCodeFetchArgu16p0();
int16_t zeroCodeFetchArgs16p0();

template <typename T> static void zeroStackSaveLocalPop(uint32_t l)
{
    auto ll = reinterpret_cast<uintptr_t>(thisThread.metadata->zero.frame) - (l + 1) * sizeof(void *);
    *reinterpret_cast<T *>(ll) = zeroStackPopGet<T>();
}

template <typename T> static void zeroStackSaveLocal(uint32_t l, T data)
{
    auto ll = reinterpret_cast<uintptr_t>(thisThread.metadata->zero.frame) - (l + 1) * sizeof(void *);
    *reinterpret_cast<T *>(ll) = data;
}

template <typename T> static T zeroStackLoadLocal(uint32_t l)
{
    auto ll = reinterpret_cast<uintptr_t>(thisThread.metadata->zero.frame) - (l + 1) * sizeof(void *);
    return *reinterpret_cast<T *>(ll);
}

class OMElysiaExecutorZero
{
  public:
    OMElysiaExecutorZero(OMElysiaVirtualWorld *vw);
    ~OMElysiaExecutorZero();

    void execute(OMElysiaMethod *m);
    void pushFrame(OMElysiaMethod *m);
    void popFrame();

  private:
    OMElysiaVirtualWorld *world;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::elysia::executor

#endif
