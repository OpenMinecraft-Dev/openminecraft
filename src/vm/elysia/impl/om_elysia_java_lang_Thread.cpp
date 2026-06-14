#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    void Java_java_lang_Thread_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
    }

    OMElysiaNativeHandle *Java_java_lang_Thread_currentThread(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        return thisThread.metadata->threadObject;
    }
}
} // namespace openminecraft::vm::elysia::impl
