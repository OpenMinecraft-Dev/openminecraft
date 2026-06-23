#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    OMElysiaNativeHandle *Java_java_lang_Thread_currentThread(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        return thisThread.metadata->threadObject;
    }

    void Java_java_lang_Thread_setPriority0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *thread, int priority)
    {
    }

    jboolean Java_java_lang_Thread_isAlive(OMElysiaJNIEnv *env, OMElysiaNativeHandle *thread)
    {
        auto thrcls = env->FindClass("java/lang/Thread");
        auto pthr = (OMElysiaThread *)env->GetLongField(thread, env->GetFieldID(thrcls, "eetop", "J"));
        return pthr && pthr->threadInited;
    }

    void Java_java_lang_Thread_start0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *thread)
    {
        env->internal->elysium->startThread(thread);
    }

    void Java_java_lang_Thread_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[] = {{const_cast<char *>("currentThread"), const_cast<char *>("()Ljava/lang/Thread;"),
                                      reinterpret_cast<void *>(Java_java_lang_Thread_currentThread)},
                                     {const_cast<char *>("setPriority0"), const_cast<char *>("(I)V"),
                                      reinterpret_cast<void *>(Java_java_lang_Thread_setPriority0)},
                                     {const_cast<char *>("isAlive"), const_cast<char *>("()Z"),
                                      reinterpret_cast<void *>(Java_java_lang_Thread_isAlive)},
                                     {const_cast<char *>("start0"), const_cast<char *>("()V"),
                                      reinterpret_cast<void *>(Java_java_lang_Thread_start0)}};
        env->RegisterNatives(klass, mm, 4);
    }
}
} // namespace openminecraft::vm::elysia::impl
