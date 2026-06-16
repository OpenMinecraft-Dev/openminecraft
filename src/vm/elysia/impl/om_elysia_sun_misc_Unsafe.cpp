#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/atomic/om_atomic.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include <atomic>
#include <stdexcept>

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
        if (env->internal->elysium->mainHeap.enablePtrCompress())
        {
            auto target = env->internal->elysium->oopManager->oopAccessField(handleFetch(o), offset);
            auto exp = env->internal->elysium->mainHeap.compress(handleFetch(expected));
            return atomic::atomic_cas(reinterpret_cast<uint32_t *>(target), exp,
                                      env->internal->elysium->mainHeap.compress(handleFetch(x)));
        }
        else
        {
            auto target = env->internal->elysium->oopManager->oopAccessField(handleFetch(o), offset);
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

        return env->internal->elysium->oopManager->oopHeaderLength() + ff->offset;
    }

    void Java_sun_misc_Unsafe_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[] = {{const_cast<char *>("arrayBaseOffset"), const_cast<char *>("(Ljava/lang/Class;)I"),
                                      reinterpret_cast<void *>(Java_sun_misc_Unsafe_arrayBaseOffset)},
                                     {const_cast<char *>("arrayIndexScale"), const_cast<char *>("(Ljava/lang/Class;)I"),
                                      reinterpret_cast<void *>(Java_sun_misc_Unsafe_arrayIndexScale)},
                                     {const_cast<char *>("addressSize"), const_cast<char *>("()I"),
                                      reinterpret_cast<void *>(Java_sun_misc_Unsafe_addressSize)},
                                     {const_cast<char *>("compareAndSwapObject"),
                                      const_cast<char *>("(Ljava/lang/Object;JLjava/lang/Object;Ljava/lang/Object;)Z"),
                                      reinterpret_cast<void *>(Java_sun_misc_Unsafe_compareAndSwapObject)},
                                     {const_cast<char *>("objectFieldOffset"),
                                      const_cast<char *>("(Ljava/lang/reflect/Field;)J"),
                                      reinterpret_cast<void *>(Java_sun_misc_Unsafe_objectFieldOffset)}};
        env->RegisterNatives(klass, mm, 5);
    }
}
} // namespace openminecraft::vm::elysia::impl
