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
        auto kk = env->FindClass("java/util/Hashtable");
        auto kkm = env->GetMethodID(kk, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

        OMElysiaNativeValue args[2];
        args[0].l = env->NewStringUTF("file.encoding");
        args[1].l = env->NewStringUTF("UTF_8");
        env->CallObjectMethodA(properties, kkm, args);
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

    void Java_java_lang_System_setIn0(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaNativeHandle *in)
    {
        env->SetStaticObjectField(klass, env->GetFieldID(klass, "in", "Ljava/io/InputStream;"), in);
    }

    void Java_java_lang_System_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[] = {
            {const_cast<char *>("initProperties"), const_cast<char *>("(Ljava/util/Properties;)Ljava/util/Properties;"),
             reinterpret_cast<void *>(Java_java_lang_System_initProperties)},
            {const_cast<char *>("arraycopy"), const_cast<char *>("(Ljava/lang/Object;ILjava/lang/Object;II)V"),
             reinterpret_cast<void *>(Java_java_lang_System_arraycopy)},
            {const_cast<char *>("setIn0"), const_cast<char *>("(Ljava/io/InputStream;)V"),
             reinterpret_cast<void *>(Java_java_lang_System_setIn0)}};
        env->RegisterNatives(klass, mm, 3);
    }
}
} // namespace openminecraft::vm::elysia::impl
