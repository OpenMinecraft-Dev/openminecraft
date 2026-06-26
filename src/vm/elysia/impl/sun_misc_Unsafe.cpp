#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/atomic/om_atomic.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include <cstdint>

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    static jint arrayBaseOffset(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd, OMElysiaNativeHandle *klass)
    {
        return static_cast<jint>(env->internal->elysium->oopManager->oopArrayHeaderLength());
    }

    static jint arrayIndexScale(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd, OMElysiaNativeHandle *klass)
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

    static jint addressSize(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
    {
        return sizeof(void *);
    }

    static jboolean compareAndSwapObject(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd, OMElysiaNativeHandle *o,
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

    static jlong objectFieldOffset(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *field)
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

    static jint getIntVolatile(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *obj, jlong n)
    {
        return *reinterpret_cast<volatile jint *>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + n);
    }

    static jlong getLongVolatile(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *obj,
                                 jlong n)
    {
        return *reinterpret_cast<volatile jlong *>(reinterpret_cast<uintptr_t>(handleFetch(obj)) + n);
    }

    static jint getInt$obj(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *obj, jlong n)
    {
        return *reinterpret_cast<jint *>(reinterpret_cast<uintptr_t>(obj) + n);
    }

    static bool compareAndSwapInt(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *o,
                                  jlong offset, jint expected, jint x)
    {
        return atomic::atomic_cas(reinterpret_cast<jint *>(reinterpret_cast<uintptr_t>(handleFetch(o)) + offset),
                                  expected, x);
    }

    static jlong allocateMemory(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong l)
    {
        return (jlong)mem::allocator::tracedMallocElysiaExternal(l);
    }

    static jlong getLong$obj(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *o, jlong off)
    {
        return *reinterpret_cast<jlong *>(reinterpret_cast<uintptr_t>(handleFetch(o)) + off);
    }

    static void putLong(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong addr, jlong v)
    {
        *(jlong *)addr = v;
    }

    static jbyte getByte(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong addr)
    {
        return *(jbyte *)addr;
    }

    static void freeMemory(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong addr)
    {
        mem::allocator::tracedFreeElysiaExternal((void *)addr);
    }

    static OMElysiaNativeHandle *getObjectVolatile(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance,
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

    static bool compareAndSwapLong(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *o,
                                   jlong offset, jlong expected, jlong x)
    {
        return atomic::atomic_cas(reinterpret_cast<jlong *>(reinterpret_cast<uintptr_t>(handleFetch(o)) + offset),
                                  expected, x);
    }

    static void putLong$obj(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *o, jlong offset,
                            jlong v)
    {
        *reinterpret_cast<jlong *>(reinterpret_cast<uintptr_t>(handleFetch(o)) + offset) = v;
    }

    static void putInt$obj(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *o, jlong offset,
                           jint v)
    {
        *reinterpret_cast<jint *>(reinterpret_cast<uintptr_t>(handleFetch(o)) + offset) = v;
    }

    void Java_sun_misc_Unsafe_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        interface::registerNativeFuncs(
            env, klass,
            {
                {"arrayBaseOffset", "(Ljava/lang/Class;)I", arrayBaseOffset},
                {"arrayIndexScale", "(Ljava/lang/Class;)I", arrayIndexScale},
                {"addressSize", "()I", addressSize},
                {"objectFieldOffset", "(Ljava/lang/reflect/Field;)J", objectFieldOffset},
                {"getIntVolatile", "(Ljava/lang/Object;J)I", getIntVolatile},
                {"getLongVolatile", "(Ljava/lang/Object;J)J", getLongVolatile},
                {"getObjectVolatile", "(Ljava/lang/Object;J)Ljava/lang/Object;", getObjectVolatile},
                {"compareAndSwapInt", "(Ljava/lang/Object;JII)Z", compareAndSwapInt},
                {"compareAndSwapLong", "(Ljava/lang/Object;JJJ)Z", compareAndSwapLong},
                {"compareAndSwapObject", "(Ljava/lang/Object;JLjava/lang/Object;Ljava/lang/Object;)Z",
                 compareAndSwapObject},
                {"getByte", "(J)B", getByte},
                {"putLong", "(JJ)V", putLong},
                {"getInt", "(Ljava/lang/Object;J)I", getInt$obj},
                {"putInt", "(Ljava/lang/Object;JI)V", putInt$obj},
                {"getLong", "(Ljava/lang/Object;J)J", getLong$obj},
                {"putLong", "(Ljava/lang/Object;JJ)V", putLong$obj},
                {"allocateMemory", "(J)J", allocateMemory},
                {"freeMemory", "(J)V", freeMemory},
            });
    }
}
} // namespace openminecraft::vm::elysia::impl
