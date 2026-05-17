#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include <stdexcept>

namespace openminecraft::vm::elysia::impl
{
log::OMLogger logger("Elysia Impl Layer");

OMElysiaNativeHandle *Java_java_lang_System_initProperties(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                           OMElysiaNativeHandle *properties)
{
    return properties;
}

void Java_java_lang_System_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    OMElysiaNativeMethod mm[] = {{const_cast<char *>("initProperties"), const_cast<char *>("(Ljava/util/Properties;)V"),
                                  reinterpret_cast<void *>(Java_java_lang_System_initProperties)}};
    env->RegisterNatives(klass, mm, 1);
}

void Java_java_lang_Object_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
}

OMElysiaNativeHandle *Java_java_lang_Class_getPrimitiveClass(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                             OMElysiaNativeHandle *name)
{
    auto nn = env->GetStringUTFChars(name, nullptr);
    auto k2 = env->FindClass(nn);
    env->ReleaseStringUTFChars(name, nn);
    auto ff =
        reinterpret_cast<OMElysiaNativeHandle *>(mem::allocator::tracedMallocElysia(sizeof(OMElysiaNativeHandle)));
    ff->next = ff;
    ff->object = k2->mirror;
    return ff;
}

// TODO: check class stat
jboolean Java_java_lang_Class_desiredAssertionStatus0(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaNativeHandle *)
{
    return true;
}

void Java_java_lang_Class_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    OMElysiaNativeMethod mm[] = {
        {const_cast<char *>("getPrimitiveClass"), const_cast<char *>("(Ljava/lang/String;)Ljava/lang/Class;"),
         reinterpret_cast<void *>(Java_java_lang_Class_getPrimitiveClass)},
        {const_cast<char *>("desiredAssertionStatus0"), const_cast<char *>("(Ljava/lang/Class;)Z"),
         reinterpret_cast<void *>(Java_java_lang_Class_desiredAssertionStatus0)}};
    env->RegisterNatives(klass, mm, 2);
}
} // namespace openminecraft::vm::elysia::impl
