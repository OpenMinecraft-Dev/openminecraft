#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    OMElysiaNativeHandle *Java_java_lang_ClassLoader_findBuiltinLib(OMElysiaJNIEnv *env, OMElysiaKlass *,
                                                                    OMElysiaNativeHandle *name)
    {
        return name;
    }
    void Java_java_lang_ClassLoader_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[] = {{const_cast<char *>("findBuiltinLib"),
                                      const_cast<char *>("(Ljava/lang/String;)Ljava/lang/String;"),
                                      reinterpret_cast<void *>(Java_java_lang_ClassLoader_findBuiltinLib)}};
        env->RegisterNatives(klass, mm, 1);
    }
}
} // namespace openminecraft::vm::elysia::impl
