#include "fmt/format.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/atomic/om_atomic.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace openminecraft::vm::elysia::impl
{
static jint arrayBaseOffset(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd, OMElysiaNativeHandle *klass)
{
    return static_cast<jint>(env->internal->elysium->oopManager->oopArrayHeaderLength());
}

static jint arrayIndexScale(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd, OMElysiaNativeHandle *klass)
{
    using namespace binary::hash;

    auto kls =
        ((OMElysiaKlass *)env->GetLongField(klass, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
    jint i = 0;
    switch (hash_compile_time(kls->name))
    {
    case "[Z"_hash:
    case "[B"_hash:
        i = 1;
        break;
    case "[C"_hash:
    case "[S"_hash:
        i = 2;
        break;
    case "[I"_hash:
    case "[F"_hash:
        i = 4;
        break;
    case "[J"_hash:
    case "[D"_hash:
        i = 8;
        break;
    default:
        i = env->internal->elysium->mainHeap.ptrLength();
        break;
    }
    return i;
}

static jint addressSize(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
{
    return sizeof(void *);
}

static jlong objectFieldOffset(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *field)
{
    auto fldkls = env->FindClass("java/lang/reflect/Field");
    auto namestr = env->GetObjectField(field, env->GetFieldID(fldkls, "name", "Ljava/lang/String;"));
    auto kls = env->GetObjectField(field, env->GetFieldID(fldkls, "clazz", "Ljava/lang/Class;"));
    auto nnstr = env->GetStringUTFChars(namestr, nullptr);
    auto ik = (OMElysiaKlass *)env->GetLongField(kls, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J"));
    auto ff = ik->toInstance()->findField(nnstr, nullptr);
    env->ReleaseStringUTFChars(namestr, nnstr);

    return env->internal->elysium->oopManager->oopAccessField(handleFetch(instance), ff->offset) -
           reinterpret_cast<uintptr_t>(handleFetch(instance));
}

static jlong staticFieldOffset(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *field)
{
    auto fldkls = env->FindClass("java/lang/reflect/Field");
    auto namestr = env->GetObjectField(field, env->GetFieldID(fldkls, "name", "Ljava/lang/String;"));
    auto kls = env->GetObjectField(field, env->GetFieldID(fldkls, "clazz", "Ljava/lang/Class;"));
    auto nnstr = env->GetStringUTFChars(namestr, nullptr);
    auto ik = (OMElysiaKlass *)env->GetLongField(kls, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J"));
    auto ff = ik->toInstance()->findField(nnstr, nullptr);
    env->ReleaseStringUTFChars(namestr, nnstr);

    return reinterpret_cast<uintptr_t>(ik->toInstance()->staticBlock) + ff->offset - reinterpret_cast<uintptr_t>(kls);
}

static OMElysiaNativeHandle *staticFieldBase(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
                                             OMElysiaNativeHandle *field)
{
    auto fldkls = env->FindClass("java/lang/reflect/Field");
    auto namestr = env->GetObjectField(field, env->GetFieldID(fldkls, "name", "Ljava/lang/String;"));
    auto kls = env->GetObjectField(field, env->GetFieldID(fldkls, "clazz", "Ljava/lang/Class;"));
    auto nnstr = env->GetStringUTFChars(namestr, nullptr);
    auto ik = (OMElysiaKlass *)env->GetLongField(kls, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J"));
    auto kkinstance = ik->mirror;
    env->ReleaseStringUTFChars(namestr, nnstr);

    return createTempHandle(kkinstance);
}

static jlong allocateMemory(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong l)
{
    return (jlong)mem::allocator::tracedMallocElysiaExternal(l);
}

static jlong reallocateMemory(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong l, jlong siz)
{
    return (jlong)mem::allocator::tracedReallocElysia((void *)l, (size_t)siz);
}

static void freeMemory(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong addr)
{
    mem::allocator::tracedFreeElysiaExternal((void *)addr);
}

static void setMemory(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *obj, jlong offset,
                      jlong bytes, jbyte value)
{
    std::memset(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + offset), value, bytes);
}

static void copyMemory(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *objsrc, jlong offsrc,
                       OMElysiaNativeHandle *objdst, jlong offdst, jlong siz)
{
    std::memcpy(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(handleFetch(objdst)) + offdst),
                reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(handleFetch(objsrc)) + offsrc), siz);
}

template <typename V> static void putDirect(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong addr, V v)
{
    *(V *)addr = v;
}
template <typename V> static V getDirect(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong addr)
{
    return *(V *)addr;
}

static void putAddr(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong addr, jlong v)
{
    *(uintptr_t *)addr = (uintptr_t)v;
}
static jlong getAddr(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong addr)
{
    return (jlong) * (uintptr_t *)addr;
}

template <typename V>
static V getVolatile(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *obj, jlong offset)
{
    return *reinterpret_cast<volatile V *>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + offset);
}
static OMElysiaNativeHandle *getVolatileObject(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
                                               OMElysiaNativeHandle *obj, jlong offset)
{
    if (env->internal->elysium->mainHeap.enablePtrCompress())
    {
        return createTempHandle(reinterpret_cast<OMElysiaOop *>(env->internal->elysium->mainHeap.decompress(
            *reinterpret_cast<volatile uint32_t *>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + offset))));
    }
    else
    {
        return createTempHandle(
            *reinterpret_cast<OMElysiaOop *volatile *>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + offset));
    }
}
template <typename V>
static void putVolatile(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *obj, jlong offset,
                        V v)
{
    *reinterpret_cast<volatile V *>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + offset) = v;
}
static void putVolatileObject(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *obj,
                              jlong offset, OMElysiaNativeHandle *v)
{
    if (env->internal->elysium->mainHeap.enablePtrCompress())
    {
        *reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + offset) =
            env->internal->elysium->mainHeap.compress(handleFetch(v));
    }
    else
    {
        *reinterpret_cast<OMElysiaOop **>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + offset) = handleFetch(v);
    }
}
template <typename V>
static V getObject(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *obj, jlong n)
{
    return *reinterpret_cast<V *>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + n);
}

static OMElysiaNativeHandle *getObjectObject(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
                                             OMElysiaNativeHandle *obj, jlong offset)
{
    if (env->internal->elysium->mainHeap.enablePtrCompress())
    {
        return createTempHandle(reinterpret_cast<OMElysiaOop *>(env->internal->elysium->mainHeap.decompress(
            *reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + offset))));
    }
    else
    {
        return createTempHandle(
            *reinterpret_cast<OMElysiaOop **>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + offset));
    }
}

template <typename V>
static void putObject(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *object, jlong offset,
                      V v)
{
    *reinterpret_cast<V *>(reinterpret_cast<uintptr_t>(handleFetch(object)) + offset) = v;
}

static void putObjectObject(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *obj,
                            jlong offset, OMElysiaNativeHandle *v)
{
    if (env->internal->elysium->mainHeap.enablePtrCompress())
    {
        *reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + offset) =
            env->internal->elysium->mainHeap.compress(handleFetch(v));
    }
    else
    {
        *reinterpret_cast<OMElysiaOop **>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + offset) = handleFetch(v);
    }
}

template <typename V>
static bool compareAndSwap(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *o, jlong offset,
                           V expected, V x)
{
    return atomic::atomic_cas(reinterpret_cast<V *>(reinterpret_cast<uintptr_t>(handleFetch(o)) + offset), expected, x);
}

static jboolean compareAndSwapObject(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *o,
                                     jlong offset, OMElysiaNativeHandle *expected, OMElysiaNativeHandle *x)
{
    auto target = reinterpret_cast<uintptr_t>(handleFetch(o)) + offset;
    if (env->internal->elysium->mainHeap.enablePtrCompress())
    {
        auto exp = env->internal->elysium->mainHeap.compress(handleFetch(expected));
        return atomic::atomic_cas(reinterpret_cast<uint32_t *>(target), exp,
                                  env->internal->elysium->mainHeap.compress(handleFetch(x)));
    }
    else
    {
        auto exp = handleFetch(expected);
        return atomic::atomic_cas(reinterpret_cast<OMElysiaOop **>(target), exp, handleFetch(x));
    }
}

static OMElysiaNativeHandle *allocateInstance(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
                                              OMElysiaNativeHandle *klass)
{
    auto ik =
        (OMElysiaKlass *)env->GetLongField(klass, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J"));
    return env->AllocObject(ik);
}

static void monitorEnter(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *obj)
{
    env->MonitorEnter(obj);
}

static void monitorExit(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *obj)
{
    env->MonitorExit(obj);
}

static jboolean tryMonitorEnter(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *obj)
{
    return env->internal->elysium->monitorManager->mutexTryFetch(handleFetch(obj));
}

static void throwException(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *throwable)
{
    env->Throw(throwable);
}

static jint pageSize(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance)
{
    return os::fetchPageSize();
}

static jint getLoadAverage(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *arr, jint len)
{
    auto l = env->GetDoubleArrayElements(arr, nullptr);
    auto result = os::fetchLoadAverage(l, len);
    env->ReleaseDoubleArrayElements(arr, l, 0);
    return result;
}

static jboolean shouldBeInitialized(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *klass)
{
    auto kls =
        ((OMElysiaKlass *)env->GetLongField(klass, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
    if (kls->isPrimitive())
    {
        return false;
    }

    return !kls->toInstance()->clinitFinished;
}

static OMElysiaNativeHandle *defineAnonymousClass(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
                                                  OMElysiaNativeHandle *host, OMElysiaNativeHandle *bytearr,
                                                  OMElysiaNativeHandle *cpPatches)
{
    auto k = (OMElysiaKlass *)env->GetLongField(host, interface::field(env, "java/lang/Class", "<ptr>", "J"));
    auto barr = env->GetByteArrayElements(bytearr, nullptr);
    auto kls = env->internal->elysium->klassLoader->loadClassWithoutMirror(
        std::make_shared<std::istringstream>(std::string((char *)barr, env->GetArrayLength(bytearr))), false,
        fmt::format("{}$$Lambda/{}", k->name, (void *)handleFetch(cpPatches)));
    env->ReleaseByteArrayElements(bytearr, barr, 0);
    env->internal->elysium->klassLoader->fixClassMirror(kls);
    return createTempHandle(kls->mirror);
}

static void ensureClassInitialized(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *klass)
{
    auto kls =
        ((OMElysiaKlass *)env->GetLongField(klass, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
    env->internal->elysium->klassLoader->ensureClassInit(kls);
}

extern "C"
{
    void Java_sun_misc_Unsafe_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        interface::registerNativeFuncs(
            env, klass,
            {
                {"arrayBaseOffset", "(Ljava/lang/Class;)I", arrayBaseOffset},
                {"arrayIndexScale", "(Ljava/lang/Class;)I", arrayIndexScale},
                {"addressSize", "()I", addressSize},
                {"objectFieldOffset", "(Ljava/lang/reflect/Field;)J", objectFieldOffset},
                {"staticFieldOffset", "(Ljava/lang/reflect/Field;)J", staticFieldOffset},
                {"staticFieldBase", "(Ljava/lang/reflect/Field;)Ljava/lang/Object;", staticFieldBase},
                {"getByteVolatile", "(Ljava/lang/Object;J)B", getVolatile<jbyte>},
                {"putByteVolatile", "(Ljava/lang/Object;JB)V", putVolatile<jbyte>},
                {"getCharVolatile", "(Ljava/lang/Object;J)C", getVolatile<jchar>},
                {"putCharVolatile", "(Ljava/lang/Object;JC)V", putVolatile<jchar>},
                {"getBooleanVolatile", "(Ljava/lang/Object;J)Z", getVolatile<jboolean>},
                {"putBooleanVolatile", "(Ljava/lang/Object;JZ)V", putVolatile<jboolean>},
                {"getShortVolatile", "(Ljava/lang/Object;J)S", getVolatile<jshort>},
                {"putShortVolatile", "(Ljava/lang/Object;JS)V", putVolatile<jshort>},
                {"getFloatVolatile", "(Ljava/lang/Object;J)F", getVolatile<jfloat>},
                {"putFloatVolatile", "(Ljava/lang/Object;JF)V", putVolatile<jfloat>},
                {"getIntVolatile", "(Ljava/lang/Object;J)I", getVolatile<jint>},
                {"putIntVolatile", "(Ljava/lang/Object;JI)V", putVolatile<jint>},
                {"getDoubleVolatile", "(Ljava/lang/Object;J)D", getVolatile<jdouble>},
                {"putDoubleVolatile", "(Ljava/lang/Object;JD)V", putVolatile<jdouble>},
                {"getLongVolatile", "(Ljava/lang/Object;J)J", getVolatile<jlong>},
                {"putLongVolatile", "(Ljava/lang/Object;JJ)V", putVolatile<jlong>},
                {"getObjectVolatile", "(Ljava/lang/Object;J)Ljava/lang/Object;", getVolatileObject},
                {"putObjectVolatile", "(Ljava/lang/Object;JLjava/lang/Object;)V", putVolatileObject},
                {"compareAndSwapInt", "(Ljava/lang/Object;JII)Z", compareAndSwap<jint>},
                {"compareAndSwapLong", "(Ljava/lang/Object;JJJ)Z", compareAndSwap<jlong>},
                {"compareAndSwapObject", "(Ljava/lang/Object;JLjava/lang/Object;Ljava/lang/Object;)Z",
                 compareAndSwapObject},
                {"getByte", "(J)B", getDirect<jbyte>},
                {"putByte", "(JB)V", putDirect<jbyte>},
                {"getChar", "(J)C", getDirect<jchar>},
                {"putChar", "(JC)V", putDirect<jchar>},
                {"getBoolean", "(J)Z", getDirect<jboolean>},
                {"putBoolean", "(JZ)V", putDirect<jboolean>},
                {"getShort", "(J)S", getDirect<jshort>},
                {"putShort", "(JS)V", putDirect<jshort>},
                {"getFloat", "(J)F", getDirect<jfloat>},
                {"putFloat", "(JF)V", putDirect<jfloat>},
                {"getInt", "(J)I", getDirect<jint>},
                {"putInt", "(JI)V", putDirect<jint>},
                {"getDouble", "(J)D", getDirect<jdouble>},
                {"putDouble", "(JD)V", putDirect<jdouble>},
                {"getLong", "(J)J", getDirect<jlong>},
                {"putLong", "(JJ)V", putDirect<jlong>},
                {"getAddress", "(J)J", getAddr},
                {"putAddress", "(JJ)V", putAddr},
                {"getByte", "(Ljava/lang/Object;J)B", getObject<jbyte>},
                {"putByte", "(Ljava/lang/Object;JB)V", putObject<jbyte>},
                {"getChar", "(Ljava/lang/Object;J)C", getObject<jchar>},
                {"putChar", "(Ljava/lang/Object;JC)V", putObject<jchar>},
                {"getBoolean", "(Ljava/lang/Object;J)Z", getObject<jboolean>},
                {"putBoolean", "(Ljava/lang/Object;JZ)V", putObject<jboolean>},
                {"getShort", "(Ljava/lang/Object;J)S", getObject<jshort>},
                {"putShort", "(Ljava/lang/Object;JS)V", putObject<jshort>},
                {"getFloat", "(Ljava/lang/Object;J)F", getObject<jfloat>},
                {"putFloat", "(Ljava/lang/Object;JF)V", putObject<jfloat>},
                {"getInt", "(Ljava/lang/Object;J)I", getObject<jint>},
                {"putInt", "(Ljava/lang/Object;JI)V", putObject<jint>},
                {"getDouble", "(Ljava/lang/Object;J)D", getObject<jdouble>},
                {"putDouble", "(Ljava/lang/Object;JD)V", putObject<jdouble>},
                {"getLong", "(Ljava/lang/Object;J)J", getObject<jlong>},
                {"putLong", "(Ljava/lang/Object;JJ)V", putObject<jlong>},
                {"getObject", "(Ljava/lang/Object;J)Ljava/lang/Object;", getObjectObject},
                {"putObject", "(Ljava/lang/Object;JLjava/lang/Object;)V", putObjectObject},
                {"allocateMemory", "(J)J", allocateMemory},
                {"reallocateMemory", "(JJ)J", reallocateMemory},
                {"freeMemory", "(J)V", freeMemory},
                {"setMemory", "(Ljava/lang/Object;JJB)V", setMemory},
                {"copyMemory", "(Ljava/lang/Object;JLjava/lang/Object;JJ)V", copyMemory},
                {"allocateInstance", "(Ljava/lang/Class;)Ljava/lang/Object;", allocateInstance},
                {"monitorEnter", "(Ljava/lang/Object;)V", monitorEnter},
                {"monitorExit", "(Ljava/lang/Object;)V", monitorExit},
                {"tryMonitorEnter", "(Ljava/lang/Object;)Z", tryMonitorEnter},
                {"throwException", "(Ljava/lang/Throwable;)V", throwException},
                {"pageSize", "()I", pageSize},
                {"getLoadAverage", "([DI)I", getLoadAverage},
                {"shouldBeInitialized", "(Ljava/lang/Class;)Z", shouldBeInitialized},
                {"defineAnonymousClass", "(Ljava/lang/Class;[B[Ljava/lang/Object;)Ljava/lang/Class;",
                 defineAnonymousClass},
                {"ensureClassInitialized", "(Ljava/lang/Class;)V", ensureClassInitialized},
            });
    }
}
} // namespace openminecraft::vm::elysia::impl
