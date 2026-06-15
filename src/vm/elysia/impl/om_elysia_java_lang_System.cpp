#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    OMElysiaNativeHandle *Java_java_lang_System_initProperties(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                               OMElysiaNativeHandle *properties)
    {
        return properties;
    }

    void Java_java_lang_System_arraycopy(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaNativeHandle *src,
                                         jint srcPos, OMElysiaNativeHandle *dst, jint dstPos, jint length)
    {
        auto kk = env->GetObjectClass(src)->toArray()->itemLength;
        auto srcraw = env->internal->elysium->oopManager->arrAccess<uint8_t>(handleFetch(src));
        auto dstraw = env->internal->elysium->oopManager->arrAccess<uint8_t>(handleFetch(dst));
        std::memmove(&dstraw[kk * dstPos], &srcraw[kk * srcPos], kk * length);
    }

    void Java_java_lang_System_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[] = {
            {const_cast<char *>("initProperties"), const_cast<char *>("(Ljava/util/Properties;)Ljava/util/Properties;"),
             reinterpret_cast<void *>(Java_java_lang_System_initProperties)},
            {const_cast<char *>("arraycopy"), const_cast<char *>("(Ljava/lang/Object;ILjava/lang/Object;II)V"),
             reinterpret_cast<void *>(Java_java_lang_System_arraycopy)}};
        env->RegisterNatives(klass, mm, 2);
    }
}
} // namespace openminecraft::vm::elysia::impl
