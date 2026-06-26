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
    static jint hashCode(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
    {
        return static_cast<jint>(reinterpret_cast<uintptr_t>(handleFetch(hnd)));
    }

    static OMElysiaNativeHandle *getClass(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
    {
        return createTempHandle(env->GetObjectClass(hnd)->mirror);
    }

    static void notifyAll(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
    {
        log::OMLogger logger("java/lang/Object", handleFetch(hnd));
        logger.warn("notifyAll not implemented!");
    }

    static OMElysiaNativeHandle *clone(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
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
                                           {"hashCode", "()I", hashCode},
                                           {"getClass", "()Ljava/lang/Class;", getClass},
                                           {"notifyAll", "()V", notifyAll},
                                           {"clone", "()Ljava/lang/Object;", clone},
                                       });
    }
}
} // namespace openminecraft::vm::elysia::impl
