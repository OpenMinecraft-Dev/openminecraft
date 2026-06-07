#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    OMElysiaNativeHandle *Java_java_lang_System_initProperties(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                               OMElysiaNativeHandle *properties)
    {
        return properties;
    }

    void Java_java_lang_System_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[] = {{const_cast<char *>("initProperties"),
                                      const_cast<char *>("(Ljava/util/Properties;)Ljava/util/Properties;"),
                                      reinterpret_cast<void *>(Java_java_lang_System_initProperties)}};
        env->RegisterNatives(klass, mm, 1);
    }
}
} // namespace openminecraft::vm::elysia::impl
