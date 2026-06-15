#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    static jint Java_java_lang_Object_hashCode(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
    {
        return static_cast<jint>(reinterpret_cast<uintptr_t>(handleFetch(hnd)));
    }

    void Java_java_lang_Object_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[] = {{const_cast<char *>("hashCode"), const_cast<char *>("()I"),
                                      reinterpret_cast<void *>(Java_java_lang_Object_hashCode)}};
        env->RegisterNatives(klass, mm, 1);
    }
}
} // namespace openminecraft::vm::elysia::impl
