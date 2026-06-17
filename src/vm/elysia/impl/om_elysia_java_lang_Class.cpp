#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
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
        return createTempHandle(k2->mirror);
    }

    jboolean Java_java_lang_Class_desiredAssertionStatus0(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                          OMElysiaNativeHandle *klasshnd)
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
            if (kld->klassloader == handleFetch(klassloader))
            {
                if (kld->klassloader)
                {
                    throw std::logic_error("need to call klassloader!");
                }
                auto nn = env->GetStringUTFChars(name, nullptr);
                auto kls = kld->fetchOrLoadClass(std::string(nn));
                env->ReleaseStringUTFChars(name, nn);
                return createTempHandle(kls->mirror);
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
        auto fldkls = env->FindClass("java/lang/reflect/Field");
        auto result = env->NewObjectArray(v->fieldCount, fldkls, nullptr);
        for (int i = 0; i < v->fieldCount; i++)
        {
            auto kl = fieldDescToType(v->fields[i].desc);
            auto kls = env->FindClass(kl.c_str());

            auto fldobj = env->AllocObject(fldkls);
            env->SetObjectField(fldobj, env->GetFieldID(fldkls, "clazz", "Ljava/lang/Class;"),
                                env->internal->elysium->executor->recordLocalRef(v->mirror));
            env->SetObjectField(fldobj, env->GetFieldID(fldkls, "name", "Ljava/lang/String;"),
                                env->NewStringUTF(v->fields[i].name));
            env->SetIntField(fldobj, env->GetFieldID(fldkls, "slot", "I"), i);
            env->SetObjectField(fldobj, env->GetFieldID(fldkls, "type", "Ljava/lang/Class;"),
                                env->internal->elysium->executor->recordLocalRef(kls->mirror));
            env->SetIntField(fldobj, env->GetFieldID(fldkls, "modifiers", "I"), v->fields[i].accessFlag);

            env->SetObjectArrayElement(result, i, fldobj);
        }
        return result;
    }

    jboolean Java_java_lang_Class_isPrimitive(OMElysiaJNIEnv *env, OMElysiaNativeHandle *kls)
    {
        return ((OMElysiaKlass *)env->GetLongField(kls,
                                                   env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")))
            ->isPrimitive();
    }

    jboolean Java_java_lang_Class_isAssignableFrom(OMElysiaJNIEnv *env, OMElysiaNativeHandle *kls,
                                                   OMElysiaNativeHandle *klsother)
    {
        auto kk =
            ((OMElysiaKlass *)env->GetLongField(kls, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
        auto kkother = ((OMElysiaKlass *)env->GetLongField(
            klsother, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
        return kkother->inherits(kk);
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
             reinterpret_cast<void *>(Java_java_lang_Class_getDeclaredFields0)},
            {const_cast<char *>("isPrimitive"), const_cast<char *>("()Z"),
             reinterpret_cast<void *>(Java_java_lang_Class_isPrimitive)},
            {const_cast<char *>("isAssignableFrom"), const_cast<char *>("(Ljava/lang/Class;)Z"),
             reinterpret_cast<void *>(Java_java_lang_Class_isAssignableFrom)}};
        env->RegisterNatives(klass, mm, 6);
    }
}
} // namespace openminecraft::vm::elysia::impl
