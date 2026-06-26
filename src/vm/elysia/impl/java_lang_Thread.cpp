#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    static OMElysiaNativeHandle *currentThread(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        return thisThread.metadata->threadObject;
    }

    static void setPriority0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *thread, int priority)
    {
    }

    static jboolean isAlive(OMElysiaJNIEnv *env, OMElysiaNativeHandle *thread)
    {
        auto thrcls = env->FindClass("java/lang/Thread");
        auto pthr = (OMElysiaThread *)env->GetLongField(thread, env->GetFieldID(thrcls, "eetop", "J"));
        return pthr && pthr->threadInited;
    }

    static void start0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *thread)
    {
        env->internal->elysium->startThread(thread);
    }

    void Java_java_lang_Thread_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        interface::registerNativeFuncs(env, klass,
                                       {
                                           {"currentThread", "()Ljava/lang/Thread;", currentThread},
                                           {"setPriority0", "(I)V", setPriority0},
                                           {"isAlive", "()Z", isAlive},
                                           {"start0", "()V", start0},
                                       });
    }
}
} // namespace openminecraft::vm::elysia::impl
