#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/atomic/om_atomic.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    static jint Java_sun_misc_Unsafe_arrayBaseOffset(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd,
                                                     OMElysiaNativeHandle *klass)
    {
        return static_cast<jint>(env->internal->elysium->oopManager->oopArrayHeaderLength());
    }

    static jint Java_sun_misc_Unsafe_arrayIndexScale(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd,
                                                     OMElysiaNativeHandle *klass)
    {
        using namespace binary::hash;

        auto klassKlass = env->FindClass("java/lang/Class");
        auto field = env->GetFieldID(klassKlass, "name", "Ljava/lang/String;");
        auto namestring = env->GetObjectField(klass, field);
        auto clsname = env->GetStringUTFChars(namestring, nullptr);
        jint i = 0;
        switch (hash_compile_time(clsname))
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
        env->ReleaseStringUTFChars(namestring, clsname);
        return i;
    }

    static jint Java_sun_misc_Unsafe_addressSize(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
    {
        return sizeof(void *);
    }

    static jboolean Java_sun_misc_Unsafe_compareAndSwapObject(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd,
                                                              OMElysiaNativeHandle *o, jlong offset,
                                                              OMElysiaNativeHandle *expected, OMElysiaNativeHandle *x)
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

    static jlong Java_sun_misc_Unsafe_objectFieldOffset(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
                                                        OMElysiaNativeHandle *field)
    {
        auto fldkls = env->FindClass("java/lang/reflect/Field");
        auto namestr = env->GetObjectField(field, env->GetFieldID(fldkls, "name", "Ljava/lang/String;"));
        auto kls = env->GetObjectField(field, env->GetFieldID(fldkls, "clazz", "Ljava/lang/Class;"));
        auto nnstr = env->GetStringUTFChars(namestr, nullptr);
        auto ik =
            (OMElysiaKlass *)env->GetLongField(kls, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J"));
        auto ff = ik->toInstance()->findField(nnstr, nullptr);
        env->ReleaseStringUTFChars(namestr, nnstr);

        return env->internal->elysium->oopManager->oopAccessField(handleFetch(instance), ff->offset) -
               reinterpret_cast<uintptr_t>(handleFetch(instance));
    }

    static jint Java_sun_misc_Unsafe_getIntVolatile(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
                                                    OMElysiaNativeHandle *obj, jlong n)
    {
        return *reinterpret_cast<volatile jint *>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + n);
    }

    static jlong Java_sun_misc_Unsafe_getLongVolatile(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
                                                      OMElysiaNativeHandle *obj, jlong n)
    {
        return *reinterpret_cast<volatile jlong *>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + n);
    }

    static jint Java_sun_misc_Unsafe_getInt(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
                                            OMElysiaNativeHandle *obj, jlong n)
    {
        return *reinterpret_cast<jint *>(reinterpret_cast<uintptr_t>(obj) + n);
    }

    static bool Java_sun_misc_Unsafe_compareAndSwapInt(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
                                                       OMElysiaNativeHandle *o, jlong offset, jint expected, jint x)
    {
        return atomic::atomic_cas(reinterpret_cast<jint *>(reinterpret_cast<uintptr_t>(handleFetch(o)) + offset),
                                  expected, x);
    }

    static jlong Java_sun_misc_Unsafe_allocateMemory(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong l)
    {
        return (jlong)mem::allocator::tracedMallocElysiaExternal(l);
    }

    static jlong Java_sun_misc_Unsafe_getLong$obj(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
                                                  OMElysiaNativeHandle *o, jlong off)
    {
        return *reinterpret_cast<jlong *>(reinterpret_cast<uintptr_t>(handleFetch(o)) + off);
    }

    static void Java_sun_misc_Unsafe_putLong(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong addr, jlong v)
    {
        *(jlong *)addr = v;
    }

    static jbyte Java_sun_misc_Unsafe_getByte(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong addr)
    {
        return *(jbyte *)addr;
    }

    static void Java_sun_misc_Unsafe_freeMemory(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong addr)
    {
        mem::allocator::tracedFreeElysiaExternal((void *)addr);
    }

    static OMElysiaNativeHandle *Java_sun_misc_Unsafe_getObjectVolatile(OMElysiaJNIEnv *env,
                                                                        OMElysiaNativeHandle *instance,
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

    static bool Java_sun_misc_Unsafe_compareAndSwapLong(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
                                                        OMElysiaNativeHandle *o, jlong offset, jlong expected, jlong x)
    {
        return atomic::atomic_cas(reinterpret_cast<jlong *>(reinterpret_cast<uintptr_t>(handleFetch(o)) + offset),
                                  expected, x);
    }

    static void Java_sun_misc_Unsafe_putLong$obj(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
                                                 OMElysiaNativeHandle *o, jlong offset, jlong v)
    {
        *reinterpret_cast<jlong *>(reinterpret_cast<uintptr_t>(handleFetch(o)) + offset) = v;
    }

    static void Java_sun_misc_Unsafe_putInt$obj(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
                                                OMElysiaNativeHandle *o, jlong offset, jint v)
    {
        *reinterpret_cast<jint *>(reinterpret_cast<uintptr_t>(handleFetch(o)) + offset) = v;
    }

    void Java_sun_misc_Unsafe_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        interface::registerNativeFuncs(
            env, klass,
            {
                {"arrayBaseOffset", "(Ljava/lang/Class;)I", Java_sun_misc_Unsafe_arrayBaseOffset},
                {"arrayIndexScale", "(Ljava/lang/Class;)I", Java_sun_misc_Unsafe_arrayIndexScale},
                {"addressSize", "()I", Java_sun_misc_Unsafe_addressSize},
                {"compareAndSwapObject", "(Ljava/lang/Object;JLjava/lang/Object;Ljava/lang/Object;)Z",
                 Java_sun_misc_Unsafe_compareAndSwapObject},
                {"objectFieldOffset", "(Ljava/lang/reflect/Field;)J", Java_sun_misc_Unsafe_objectFieldOffset},
                {"getIntVolatile", "(Ljava/lang/Object;J)I", Java_sun_misc_Unsafe_getIntVolatile},
                {"compareAndSwapInt", "(Ljava/lang/Object;JII)Z", Java_sun_misc_Unsafe_compareAndSwapInt},
                {"allocateMemory", "(J)J", Java_sun_misc_Unsafe_allocateMemory},
                {"putLong", "(JJ)V", Java_sun_misc_Unsafe_putLong},
                {"getByte", "(J)B", Java_sun_misc_Unsafe_getByte},
                {"freeMemory", "(J)V", Java_sun_misc_Unsafe_freeMemory},
                {"getObjectVolatile", "(Ljava/lang/Object;J)Ljava/lang/Object;",
                 Java_sun_misc_Unsafe_getObjectVolatile},
                {"compareAndSwapLong", "(Ljava/lang/Object;JJJ)Z", Java_sun_misc_Unsafe_compareAndSwapLong},
                {"getInt", "(Ljava/lang/Object;J)I", Java_sun_misc_Unsafe_getInt},
                {"getLong", "(Ljava/lang/Object;J)J", Java_sun_misc_Unsafe_getLong$obj},
                {"putLong", "(Ljava/lang/Object;JJ)V", Java_sun_misc_Unsafe_putLong$obj},
                {"getLongVolatile", "(Ljava/lang/Object;J)J", Java_sun_misc_Unsafe_getLongVolatile},
                {"putInt", "(Ljava/lang/Object;JI)V", Java_sun_misc_Unsafe_putInt$obj},
            });
    }
}
} // namespace openminecraft::vm::elysia::impl
