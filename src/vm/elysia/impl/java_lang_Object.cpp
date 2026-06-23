#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    static jint Java_java_lang_Object_hashCode(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
    {
        return static_cast<jint>(reinterpret_cast<uintptr_t>(handleFetch(hnd)));
    }

    static OMElysiaNativeHandle *Java_java_lang_Object_getClass(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
    {
        return createTempHandle(env->GetObjectClass(hnd)->mirror);
    }

    void Java_java_lang_Object_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[] = {{const_cast<char *>("hashCode"), const_cast<char *>("()I"),
                                      reinterpret_cast<void *>(Java_java_lang_Object_hashCode)},
                                     {const_cast<char *>("getClass"), const_cast<char *>("()Ljava/lang/Class;"),
                                      reinterpret_cast<void *>(Java_java_lang_Object_getClass)}};
        env->RegisterNatives(klass, mm, 2);
    }
}
} // namespace openminecraft::vm::elysia::impl
