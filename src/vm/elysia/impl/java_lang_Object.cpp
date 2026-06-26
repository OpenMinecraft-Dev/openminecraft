#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
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
        interface::registerNativeFuncs(env, klass,
                                       {
                                           {"hashCode", "()I", Java_java_lang_Object_hashCode},
                                           {"getClass", "()Ljava/lang/Class;", Java_java_lang_Object_getClass},
                                           {"notifyAll", "()V", Java_java_lang_Object_notifyAll},
                                           {"clone", "()Ljava/lang/Object;", Java_java_lang_Object_clone},
                                       });
    }
}
} // namespace openminecraft::vm::elysia::impl
