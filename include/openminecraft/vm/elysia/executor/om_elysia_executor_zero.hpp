#ifndef OM_ELYSIA_EXECUTOR_ZERO_HPP
#define OM_ELYSIA_EXECUTOR_ZERO_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_field.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include "optimizations.hpp"
#include <cstdint>
#include <type_traits>
namespace openminecraft::vm::elysia::executor
{
HOT_FUNC
static inline auto zeroStackAlloc(uint64_t len) -> uintptr_t
{
    auto tc = thisThread.metadata;
    auto v = tc->zero.stackPointer -= len;
    return v;
}
HOT_FUNC
static inline auto zeroStackPop(uint64_t len) -> uintptr_t
{
    auto tc = thisThread.metadata;
    auto result = tc->zero.stackPointer;
    tc->zero.stackPointer += len;
    return result;
}

template <typename T> constexpr void assertType1()
{
    static_assert(std::is_same_v<T, jfloat> || std::is_same_v<T, jint> || std::is_same_v<T, OMElysiaOop *> ||
                      std::is_same_v<T, uint32_t> || std::is_same_v<T, void *> || std::is_same_v<T, uint8_t *>,
                  "only internal types are supported!");
}
template <typename T> constexpr void assertType1Wide()
{
    static_assert(std::is_same_v<T, jshort> || std::is_same_v<T, jchar> || std::is_same_v<T, jbyte> ||
                      std::is_same_v<T, jboolean> || std::is_same_v<T, jfloat> || std::is_same_v<T, jint> ||
                      std::is_same_v<T, OMElysiaOop *> || std::is_same_v<T, uint32_t> || std::is_same_v<T, void *> ||
                      std::is_same_v<T, uint8_t *>,
                  "only internal types are supported!");
}
template <typename T> constexpr void assertType2()
{
    static_assert(std::is_same_v<T, jlong> || std::is_same_v<T, jdouble> || std::is_same_v<T, uint64_t>,
                  "only internal types are supported!");
}

template <typename T> static inline void zeroStackPush(T data)
{
    if constexpr (std::is_same_v<T, jshort> || std::is_same_v<T, jchar> || std::is_same_v<T, jbyte> ||
                  std::is_same_v<T, jboolean>)
    {
        zeroStackPush(static_cast<jint>(data));
    }
    else
    {
        assertType1<T>();
        auto d = reinterpret_cast<uintptr_t *>(zeroStackAlloc(sizeof(void *)));
        if constexpr (std::is_pointer_v<T>)
        {
            *d = reinterpret_cast<uintptr_t>(data);
        }
        else
        {
            *reinterpret_cast<jint *>(d) = *reinterpret_cast<jint *>(&data);
        }
    }
}

void zeroStackPopToStatic(OMElysiaField *field, OMElysiaOopManager *oop, OMElysium *world);
void zeroStackPushFromStatic(OMElysiaField *field, OMElysiaOopManager *oop, OMElysium *world);
void zeroStackPopToField(OMElysiaField *field, OMElysiaOopManager *oop, OMElysium *world);
void zeroStackPushFromField(OMElysiaField *field, OMElysiaOopManager *oop, OMElysium *world);

#define zeroCodeFetchArgu16p0(pc) (static_cast<uint16_t>(pc[1] << 8) | pc[2])
#define zeroCodeFetchArgs16p0(pc) (static_cast<int16_t>(pc[1] << 8) | pc[2])
#define zeroCodeFetchArgs16p1(pc) (static_cast<int16_t>(pc[3] << 8) | pc[4])
#define zeroCodeFetchArgs32p0(pc)                                                                                      \
    (static_cast<int32_t>(pc[1] << 24) | static_cast<int32_t>(pc[2] << 16) | static_cast<int32_t>(pc[3] << 8) | pc[4])

static inline auto zeroCodeFetchArgs32Align(uint8_t *pc, int offset) -> int32_t
{
    auto pp = pc + 1;
    while (reinterpret_cast<uintptr_t>(pp) % 4)
    {
        ++pp;
    }
    return static_cast<int32_t>(pp[offset * 4] << 24) | static_cast<int32_t>(pp[offset * 4 + 1] << 16) |
           static_cast<int32_t>(pp[offset * 4 + 2] << 8) | pp[offset * 4 + 3];
}
template <typename T> static inline auto zeroStackPopGet() -> T
{
    if constexpr (std::is_same_v<T, jshort> || std::is_same_v<T, jchar> || std::is_same_v<T, jbyte> ||
                  std::is_same_v<T, jboolean>)
    {
        return *reinterpret_cast<T *>(zeroStackPop(sizeof(void *)));
    }
    else
    {
        assertType1<T>();
        return *reinterpret_cast<T *>(zeroStackPop(sizeof(void *)));
    }
}

template <typename T> static inline auto zeroStackPeekGet() -> T
{
    assertType1<T>();
    return *reinterpret_cast<T *>(thisThread.metadata->zero.stackPointer);
}

template <typename T> static inline auto zeroStackPeekGetRef() -> T *
{
    assertType1<T>();
    return reinterpret_cast<T *>(thisThread.metadata->zero.stackPointer);
}

template <typename T> static inline void zeroStackSaveLocalPop(uint32_t l, OMElysiaJavaFrame *frame)
{
    assertType1Wide<T>();
    *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(frame) - (l + 1) * sizeof(void *)) = zeroStackPopGet<T>();
}

template <typename T> static inline void zeroStackSaveLocal(uint32_t l, T data, OMElysiaJavaFrame *frame)
{
    assertType1Wide<T>();
    *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(frame) - (l + 1) * sizeof(void *)) = data;
}

template <typename T> static inline auto zeroStackLoadLocal(uint32_t l, OMElysiaJavaFrame *frame) -> T
{
    assertType1Wide<T>();
    return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(frame) - (l + 1) * sizeof(void *));
}

template <typename T> static inline auto zeroStackLoadLocalRef(uint32_t l, OMElysiaJavaFrame *frame) -> T *
{
    assertType1Wide<T>();
    return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(frame) - (l + 1) * sizeof(void *));
}

template <typename T> static inline void zeroStackPushW(T data)
{
    assertType2<T>();
    if constexpr (sizeof(void *) == 8)
    {
        *reinterpret_cast<uintptr_t *>(zeroStackAlloc(sizeof(void *))) = *reinterpret_cast<uint64_t *>(&data);
        // gino: wide data need 2 slots, on 64 bit this means one actual slot and another padding slot
        zeroStackAlloc(sizeof(void *));
    }
    else
    {
        uint64_t d = *reinterpret_cast<uint64_t *>(&data);
        auto dlow = reinterpret_cast<uint32_t *>(zeroStackAlloc(sizeof(void *)));
        auto dhigh = reinterpret_cast<uint32_t *>(zeroStackAlloc(sizeof(void *)));
        *dhigh = static_cast<uint32_t>(d >> 32);
        *dlow = static_cast<uint32_t>(d & 0xffffffff);
    }
}

template <typename T> static inline auto zeroStackPopWGet() -> T
{
    assertType2<T>();
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

template <typename T> static inline auto zeroStackLoadLocalW(uint32_t l, OMElysiaJavaFrame *frame) -> T
{
    assertType2<T>();
    if constexpr (sizeof(void *) == 8)
    {
        return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(frame) - (l + 1) * sizeof(void *));
    }
    else
    {
        auto dlow = static_cast<uint64_t>(zeroStackLoadLocal<uint32_t>(l, frame));
        auto dhigh = static_cast<uint64_t>(zeroStackLoadLocal<uint32_t>(l + 1, frame));

        auto d = dhigh << 32 | dlow;
        return *reinterpret_cast<T *>(&d);
    }
}

template <typename T> static inline void zeroStackSaveLocalPopW(uint32_t l, OMElysiaJavaFrame *frame)
{
    assertType2<T>();
    if constexpr (sizeof(void *) == 8)
    {
        *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(frame) - (l + 1) * sizeof(void *)) = zeroStackPopWGet<T>();
    }
    else
    {
        auto value = zeroStackPopWGet<T>();
        auto vv = *reinterpret_cast<uint64_t *>(&value);

        auto dlow = reinterpret_cast<uintptr_t>(frame) - (l + 1) * sizeof(void *);
        auto dhigh = reinterpret_cast<uintptr_t>(frame) - (l + 1 + 1) * sizeof(void *);

        *reinterpret_cast<uint32_t *>(dlow) = static_cast<uint32_t>(vv & 0xffffffff);
        *reinterpret_cast<uint32_t *>(dhigh) = static_cast<uint32_t>(vv >> 32);
    }
}

class OMElysiaExecutorZero
{
  public:
    OMElysiaExecutorZero(OMElysium *elysium);
    ~OMElysiaExecutorZero();

    void callVoidFunction(OMElysiaMethod *m, const OMElysiaNativeValue *args);

#define IMPL_FUNCCALL(retType, name, fetchFunc)                                                                        \
    auto call##name##Function(OMElysiaMethod *m, const OMElysiaNativeValue *args) -> retType                           \
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

    auto recordLocalRef(OMElysiaOop *) -> OMElysiaNativeHandle *;
    auto currentKlassloader() -> OMElysiaKlassloader *;
    auto findRoutine(std::string klass, std::string name) -> OMElysiaIntrinsicRoutine;

  protected:
    void execute(OMElysiaMethod *m);
    void executeNativeLink(uint8_t **realpc, OMElysiaJavaFrame *frame);
    void executeNative(OMElysiaMethod *m, bool isStatic, void *func, uint8_t **realpc, OMElysiaJavaFrame *frame);
    void pushFrame(OMElysiaMethod *m, uint8_t *retAddr, bool needVtable, uint8_t **realpc);
    void popFrame(uint8_t **realpc);

    void threadInit();

  private:
    OMElysium *elysium;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::elysia::executor

#endif
