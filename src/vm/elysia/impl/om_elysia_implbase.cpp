#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"

namespace openminecraft::vm::elysia::impl
{
log::OMLogger logger("Elysia Impl Layer");
void Java_java_lang_System_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    OMElysiaNativeMethod mm[] = {
        {"initProperties", "(Ljava/util/Properties;)V", (void *)Java_java_lang_System_initProperties}};
    env->RegisterNatives(klass, mm, 1);
}

OMElysiaOop *Java_java_lang_System_initProperties(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaOop *properties)
{
    return properties;
}

void Java_java_lang_Object_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
}

void Java_java_lang_Class_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    OMElysiaNativeMethod mm[] = {
        {"getPrimitiveClass", "(Ljava/lang/String;)Ljava/lang/Class;", (void *)Java_java_lang_Class_getPrimitiveClass}};
    env->RegisterNatives(klass, mm, 1);
}

OMElysiaOop *Java_java_lang_Class_getPrimitiveClass(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaOop *name)
{
    auto k = env->FindClass("java/lang/Class");
    auto field = env->GetFieldID(k, "name", "Ljava/lang/String;");
    auto oop = env->AllocObject(k);
    logger.info("{} @+0x{:x}", (void *)oop, field->offset);
    throw std::logic_error("not implemented");
}
} // namespace openminecraft::vm::elysia::impl
