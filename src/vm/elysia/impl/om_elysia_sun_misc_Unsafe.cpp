#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"

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

    void Java_sun_misc_Unsafe_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[] = {{const_cast<char *>("arrayBaseOffset"), const_cast<char *>("(Ljava/lang/Class;)I"),
                                      reinterpret_cast<void *>(Java_sun_misc_Unsafe_arrayBaseOffset)},
                                     {const_cast<char *>("arrayIndexScale"), const_cast<char *>("(Ljava/lang/Class;)I"),
                                      reinterpret_cast<void *>(Java_sun_misc_Unsafe_arrayIndexScale)},
                                     {const_cast<char *>("addressSize"), const_cast<char *>("()I"),
                                      reinterpret_cast<void *>(Java_sun_misc_Unsafe_addressSize)}};
        env->RegisterNatives(klass, mm, 3);
    }
}
} // namespace openminecraft::vm::elysia::impl
