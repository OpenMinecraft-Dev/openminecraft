#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <cstring>

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
        log::OMLogger logger("java/lang/Object", handleFetch(hnd));
        logger.warn("notifyAll not implemented!");
    }

    static OMElysiaNativeHandle *Java_java_lang_Object_clone(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
    {
        auto klass = env->GetObjectClass(hnd);
        auto vv = env->AllocObject(klass);
        std::memcpy(handleFetch(vv), handleFetch(hnd),
                    env->internal->elysium->oopManager->oopHeaderLength() + klass->toInstance()->length);
        return vv;
    }

    void Java_java_lang_Object_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[] = {{const_cast<char *>("hashCode"), const_cast<char *>("()I"),
                                      reinterpret_cast<void *>(Java_java_lang_Object_hashCode)},
                                     {const_cast<char *>("getClass"), const_cast<char *>("()Ljava/lang/Class;"),
                                      reinterpret_cast<void *>(Java_java_lang_Object_getClass)},
                                     {const_cast<char *>("notifyAll"), const_cast<char *>("()V"),
                                      reinterpret_cast<void *>(Java_java_lang_Object_notifyAll)},
                                     {const_cast<char *>("clone"), const_cast<char *>("()Ljava/lang/Object;"),
                                      reinterpret_cast<void *>(Java_java_lang_Object_clone)}};
        env->RegisterNatives(klass, mm, 4);
    }
}
} // namespace openminecraft::vm::elysia::impl
