#include "openminecraft/log/om_log_common.hpp"
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

    static void Java_java_lang_Object_notifyAll(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
    {
        log::OMLogger logger("java.lang.Object", handleFetch(hnd));
        logger.warn("notifyAll not implemented!");
    }

    void Java_java_lang_Object_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[] = {{const_cast<char *>("hashCode"), const_cast<char *>("()I"),
                                      reinterpret_cast<void *>(Java_java_lang_Object_hashCode)},
                                     {const_cast<char *>("getClass"), const_cast<char *>("()Ljava/lang/Class;"),
                                      reinterpret_cast<void *>(Java_java_lang_Object_getClass)},
                                     {const_cast<char *>("notifyAll"), const_cast<char *>("()V"),
                                      reinterpret_cast<void *>(Java_java_lang_Object_notifyAll)}};
        env->RegisterNatives(klass, mm, 3);
    }
}
} // namespace openminecraft::vm::elysia::impl
