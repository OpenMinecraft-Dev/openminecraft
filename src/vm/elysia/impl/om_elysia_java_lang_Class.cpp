#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <iostream>
#include <stdexcept>

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

    OMElysiaNativeHandle *Java_java_lang_Class_forName0(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                        OMElysiaNativeHandle *name, jboolean b,
                                                        OMElysiaNativeHandle *klassloader,
                                                        OMElysiaNativeHandle *loadcls)
    {
        auto kld = klass->klassloader;
        while (kld)
        {
            if (kld->klassloader == (klassloader ? klassloader->object : nullptr))
            {
                auto nn = env->GetStringUTFChars(name, nullptr);
                auto kls = kld->fetchOrLoadClass(std::string(nn));
                env->ReleaseStringUTFChars(name, nn);
                auto ff = reinterpret_cast<OMElysiaNativeHandle *>(
                    mem::allocator::tracedMallocElysia(sizeof(OMElysiaNativeHandle)));
                ff->next = ff;
                ff->object = kls->mirror;
                return ff;
            }
            kld = kld->next.get();
        }
        throw std::logic_error("klassloader not found");
    }

    OMElysiaNativeHandle *Java_java_lang_Class_getDeclaredFields0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *klass,
                                                                  bool bl)
    {
        auto kls = env->FindClass("java/lang/Class");
        auto v = ((OMElysiaKlass *)(env->GetLongField(klass, env->GetFieldID(kls, "<ptr>", "J"))))->toInstance();
        for (int i = 0; i < v->fieldCount; i++)
        {
            auto kl = fieldDescToType(v->fields[i].desc);
            env->FindClass(kl.c_str());
        }
        throw std::logic_error("not implemented");
    }

    void Java_java_lang_Class_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[] = {
            {const_cast<char *>("getPrimitiveClass"), const_cast<char *>("(Ljava/lang/String;)Ljava/lang/Class;"),
             reinterpret_cast<void *>(Java_java_lang_Class_getPrimitiveClass)},
            {const_cast<char *>("desiredAssertionStatus0"), const_cast<char *>("(Ljava/lang/Class;)Z"),
             reinterpret_cast<void *>(Java_java_lang_Class_desiredAssertionStatus0)},
            {const_cast<char *>("forName0"),
             const_cast<char *>("(Ljava/lang/String;ZLjava/lang/ClassLoader;Ljava/lang/Class;)Ljava/lang/Class;"),
             reinterpret_cast<void *>(Java_java_lang_Class_forName0)},
            {const_cast<char *>("getDeclaredFields0"), const_cast<char *>("(Z)[Ljava/lang/reflect/Field;"),
             reinterpret_cast<void *>(Java_java_lang_Class_getDeclaredFields0)}};
        env->RegisterNatives(klass, mm, 4);
    }
}
} // namespace openminecraft::vm::elysia::impl
