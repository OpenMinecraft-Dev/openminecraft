#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/atomic/om_atomic.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include <cstdint>

namespace openminecraft::vm::elysia::impl
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

static jlong allocateMemory(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong l)
{
    return (jlong)mem::allocator::tracedMallocElysiaExternal(l);
}

template <typename V> static void putDirect(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong addr, V v)
{
    *(V *)addr = v;
}
template <typename V> static V getDirect(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong addr)
{
    return *(V *)addr;
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

static void freeMemory(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, jlong addr)
{
    mem::allocator::tracedFreeElysiaExternal((void *)addr);
}

template <typename V>
static V getObject(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *obj, jlong n)
{
    return *reinterpret_cast<V *>(reinterpret_cast<uintptr_t>(obj) + n);
}

template <typename V>
static void putObject(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *object, jlong offset,
                      V v)
{
    *reinterpret_cast<V *>(reinterpret_cast<uintptr_t>(handleFetch(object)) + offset) = v;
}

template <typename V>
static bool compareAndSwap(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance, OMElysiaNativeHandle *o, jlong offset,
                           V expected, V x)
{
    return atomic::atomic_cas(reinterpret_cast<V *>(reinterpret_cast<uintptr_t>(handleFetch(o)) + offset), expected, x);
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
                {"getInt", "(Ljava/lang/Object;J)I", getObject<jint>},
                {"putInt", "(Ljava/lang/Object;JI)V", putObject<jint>},
                {"getLong", "(Ljava/lang/Object;J)J", getObject<jlong>},
                {"putLong", "(Ljava/lang/Object;JJ)V", putObject<jlong>},
                {"allocateMemory", "(J)J", allocateMemory},
                {"freeMemory", "(J)V", freeMemory},
            });
    }
}
} // namespace openminecraft::vm::elysia::impl
