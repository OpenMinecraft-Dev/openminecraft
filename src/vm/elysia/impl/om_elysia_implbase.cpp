#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include <stdexcept>

namespace openminecraft::vm::elysia::impl
{
log::OMLogger logger("Elysia Impl Layer");
void Java_java_lang_System_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    OMElysiaNativeMethod mm[] = {{const_cast<char *>("initProperties"), const_cast<char *>("(Ljava/util/Properties;)V"),
                                  reinterpret_cast<void *>(Java_java_lang_System_initProperties)}};
    env->RegisterNatives(klass, mm, 1);
}

OMElysiaNativeHandle *Java_java_lang_System_initProperties(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                           OMElysiaNativeHandle *properties)
{
    return properties;
}

void Java_java_lang_Object_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
}

void Java_java_lang_Class_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    OMElysiaNativeMethod mm[] = {{const_cast<char *>("getPrimitiveClass"),
                                  const_cast<char *>("(Ljava/lang/String;)Ljava/lang/Class;"),
                                  reinterpret_cast<void *>(Java_java_lang_Class_getPrimitiveClass)}};
    env->RegisterNatives(klass, mm, 1);
}

OMElysiaNativeHandle *Java_java_lang_Class_getPrimitiveClass(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                             OMElysiaNativeHandle *name)
{
    auto k = env->FindClass("java/lang/Class");
    auto field = env->GetFieldID(k, "name", "Ljava/lang/String;");
    auto oop = env->AllocObject(k);

    auto nn = env->GetStringUTFChars(name, nullptr);
    auto k2 = env->FindClass(nn);
    env->ReleaseStringUTFChars(name, nn);

    logger.info("{} @+0x{:x} {}", (void *)oop, field->offset, (void *)k2);

    throw std::logic_error("not implemented");
}
} // namespace openminecraft::vm::elysia::impl
