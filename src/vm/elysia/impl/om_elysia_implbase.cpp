#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"

namespace openminecraft::vm::elysia::impl
{
void Java_java_lang_System_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    OMElysiaNativeMethod mm[1] = {
        {"initProperties", "(Ljava/util/Properties;)V", (void *)Java_java_lang_System_initProperties}};
    (*env)->RegisterNatives(env, klass, mm, 1);
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
}

OMElysiaOop *Java_java_lang_Class_getPrimitiveClass(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaOop* name)
{
    throw std::logic_error("not implemented");
}
} // namespace openminecraft::vm::elysia::impl
