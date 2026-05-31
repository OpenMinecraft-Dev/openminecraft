#ifndef OM_ELYSIA_EXECUTOR_ZERO_HPP
#define OM_ELYSIA_EXECUTOR_ZERO_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_field.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <cstdarg>
#include <cstdint>
#include <type_traits>
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

        // *d = *reinterpret_cast<uint32_t *>(&data);
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
        auto dlow = *reinterpret_cast<uint32_t *>(zeroStackPop(sizeof(void *)));
        auto dhigh = *reinterpret_cast<uint32_t *>(zeroStackPop(sizeof(void *)));

        auto d = static_cast<uint64_t>(dhigh) << 32 | dlow;
        return *reinterpret_cast<T *>(&d);
    }
}

void zeroStackPopToStatic(OMElysiaField *field, OMElysium *world);
void zeroStackPushFromStatic(OMElysiaField *field, OMElysium *world);
void zeroStackPopToField(OMElysiaField *field, OMElysiaOopManager *oop, OMElysium *world);
void zeroStackPushFromField(OMElysiaField *field, OMElysiaOopManager *oop, OMElysium *world);
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

template <typename T> static T zeroStackLoadLocalW(uint32_t l)
{
    if constexpr (sizeof(void *) == 8)
    {
        return zeroStackLoadLocal<T>(l);
    }
    else
    {
        auto highd = static_cast<uint64_t>(zeroStackLoadLocal<uint32_t>(l));
        auto lowd = static_cast<uint64_t>(zeroStackLoadLocal<uint32_t>(l + 1));

        auto ll = highd << 32 | lowd;
        return *reinterpret_cast<T *>(&ll);
    }
}

class OMElysiaExecutorZero
{
  public:
    OMElysiaExecutorZero(OMElysium *vw);
    ~OMElysiaExecutorZero();

#define DEF_FUNCCALL(retType, name)                                                                                    \
    retType call##name##Function(OMElysiaMethod *m, const OMElysiaNativeValue *);                                      \
    retType call##name##Function(OMElysiaMethod *m, va_list);                                                          \
    retType call##name##Function(OMElysiaMethod *m, ...);

    DEF_FUNCCALL(void, Void);
    DEF_FUNCCALL(jbyte, Byte);
    DEF_FUNCCALL(jboolean, Boolean);
    DEF_FUNCCALL(jchar, Char);
    DEF_FUNCCALL(jshort, Short);
    DEF_FUNCCALL(jint, Int);
    DEF_FUNCCALL(jfloat, Float);
    DEF_FUNCCALL(jlong, Long);
    DEF_FUNCCALL(jdouble, Double);
    DEF_FUNCCALL(OMElysiaOop *, Object);

    OMElysiaNativeHandle *recordLocalRef(OMElysiaOop *);

  protected:
    void execute(OMElysiaMethod *m);
    void executeNativeLink();
    void executeNative(char *descriptor, bool isStatic, void *func);
    void pushFrame(OMElysiaMethod *m, bool needVtable);
    void popFrame();

    void threadInit();

  private:
    OMElysium *world;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::elysia::executor

#endif
