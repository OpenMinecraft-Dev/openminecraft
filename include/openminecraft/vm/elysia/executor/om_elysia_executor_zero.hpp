#ifndef OM_ELYSIA_EXECUTOR_ZERO_HPP
#define OM_ELYSIA_EXECUTOR_ZERO_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_field.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <cstdarg>
#include <cstdint>
#include <mutex>
#include <type_traits>
#include <unordered_map>
namespace openminecraft::vm::elysia::executor
{
uintptr_t zeroStackAlloc(uint64_t len);
uintptr_t zeroStackPop(uint64_t len);
template <typename T> static void zeroStackPush(T data)
{
    auto d = reinterpret_cast<uintptr_t *>(zeroStackAlloc(sizeof(void *)));
    if constexpr (std::is_pointer_v<T>)
    {
        *d = reinterpret_cast<uintptr_t>(data);
    }
    else
    {
        *d = 0;
        std::memcpy(d, &data, sizeof(T));
    }
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
        auto dlow = reinterpret_cast<uintptr_t *>(zeroStackAlloc(sizeof(void *)));
        auto dhigh = reinterpret_cast<uintptr_t *>(zeroStackAlloc(sizeof(void *)));
        *dhigh = static_cast<uint32_t>(d >> 32);
        *dlow = static_cast<uint32_t>(d & 0xffffffff);
    }
}

template <typename T> static T zeroStackPeekGet()
{
    return *reinterpret_cast<T *>(thisThread.metadata->zero.stackPointer);
}

template <typename T> static T zeroStackPopGet()
{
    T pp = *reinterpret_cast<T *>(zeroStackPop(sizeof(void *)));
    return pp;
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
        auto dhigh = *reinterpret_cast<uint32_t *>(zeroStackPop(sizeof(void *)));
        auto dlow = *reinterpret_cast<uint32_t *>(zeroStackPop(sizeof(void *)));

        auto d = static_cast<uint64_t>(dhigh) << 32 | dlow;
        return *reinterpret_cast<T *>(&d);
    }
}

void zeroStackPopToStatic(OMElysiaField *field, OMElysiaOopManager *oop, OMElysium *world);
void zeroStackPushFromStatic(OMElysiaField *field, OMElysiaOopManager *oop, OMElysium *world);
void zeroStackPopToField(OMElysiaField *field, OMElysiaOopManager *oop, OMElysium *world);
void zeroStackPushFromField(OMElysiaField *field, OMElysiaOopManager *oop, OMElysium *world);
uint16_t zeroCodeFetchArgu16p0();
int16_t zeroCodeFetchArgs16p0();
int32_t zeroCodeFetchArgs32Align(int offset);

template <typename T> static void zeroStackSaveLocalPopW(uint32_t l)
{
    if constexpr (sizeof(void *) == 8)
    {
        auto ll = reinterpret_cast<uintptr_t>(thisThread.metadata->zero.frame) - (l + 1) * sizeof(void *);
        *reinterpret_cast<T *>(ll) = zeroStackPopWGet<T>();
    }
    else
    {
        auto value = zeroStackPopWGet<T>();
        auto vv = *reinterpret_cast<uint64_t *>(&value);

        auto dlow = reinterpret_cast<uintptr_t>(thisThread.metadata->zero.frame) - (l + 1) * sizeof(void *);
        auto dhigh = reinterpret_cast<uintptr_t>(thisThread.metadata->zero.frame) - (l + 1 + 1) * sizeof(void *);

        *reinterpret_cast<uint32_t *>(dlow) = static_cast<uint32_t>(vv & 0xffffffff);
        *reinterpret_cast<uint32_t *>(dhigh) = static_cast<uint32_t>(vv >> 32);
    }
}

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

template <typename T>
static T zeroStackLoadLocal(uint32_t l, OMElysiaJavaFrame *frame = thisThread.metadata->zero.frame)
{
    auto ll = reinterpret_cast<uintptr_t>(frame) - (l + 1) * sizeof(void *);
    return *reinterpret_cast<T *>(ll);
}

template <typename T> static T zeroStackLoadLocalW(uint32_t l)
{
    if constexpr (sizeof(void *) == 8)
    {
        return zeroStackLoadLocal<T>(l);
    }
    else
    {
        auto dlow = static_cast<uint64_t>(zeroStackLoadLocal<uint32_t>(l));
        auto dhigh = static_cast<uint64_t>(zeroStackLoadLocal<uint32_t>(l + 1));

        auto d = dhigh << 32 | dlow;
        return *reinterpret_cast<T *>(&d);
    }
}

class OMElysiaExecutorZero
{
  public:
    OMElysiaExecutorZero(OMElysium *elysium);
    ~OMElysiaExecutorZero();

    void callVoidFunction(OMElysiaMethod *m, const OMElysiaNativeValue *args);

#define IMPL_FUNCCALL(retType, name, fetchFunc)                                                                        \
    retType call##name##Function(OMElysiaMethod *m, const OMElysiaNativeValue *args)                                   \
    {                                                                                                                  \
        callVoidFunction(m, args);                                                                                     \
        return fetchFunc<retType>();                                                                                   \
    }
    IMPL_FUNCCALL(jbyte, Byte, zeroStackPopGet);
    IMPL_FUNCCALL(jboolean, Boolean, zeroStackPopGet);
    IMPL_FUNCCALL(jshort, Short, zeroStackPopGet);
    IMPL_FUNCCALL(jchar, Char, zeroStackPopGet);
    IMPL_FUNCCALL(jint, Int, zeroStackPopGet);
    IMPL_FUNCCALL(jfloat, Float, zeroStackPopGet);
    IMPL_FUNCCALL(jlong, Long, zeroStackPopWGet);
    IMPL_FUNCCALL(jdouble, Double, zeroStackPopWGet);
    IMPL_FUNCCALL(OMElysiaOop *, Object, zeroStackPopGet);

    OMElysiaNativeHandle *recordLocalRef(OMElysiaOop *);
    OMElysiaKlassloader *currentKlassloader();

  protected:
    void execute(OMElysiaMethod *m);
    void executeNativeLink();
    void executeNative(char *descriptor, bool isStatic, void *func);
    void pushFrame(OMElysiaMethod *m, uint8_t *retAddr, bool needVtable);
    void popFrame();

    void threadInit();

    std::mutex objectMonitorsMutex;
    std::unordered_map<OMElysiaOop *, std::shared_ptr<std::mutex>> objectMonitors;

  private:
    OMElysium *elysium;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::elysia::executor

#endif
