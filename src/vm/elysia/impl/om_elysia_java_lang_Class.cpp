#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
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
    jboolean Java_java_lang_Class_desiredAssertionStatus0(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                          OMElysiaNativeHandle *)
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
}
} // namespace openminecraft::vm::elysia::impl
