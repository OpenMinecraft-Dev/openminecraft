#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

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
                std::string ss(nn);
                for (auto &ch : ss)
                {
                    if (ch == '.')
                    {
                        ch = '/';
                    }
                }
                auto kls = kld->fetchOrLoadClass(ss);
                env->ReleaseStringUTFChars(name, nn);

                if (env->ExceptionCheck())
                {
                    return nullptr;
                }
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
            if (bl && (v->fields[i].accessFlag & JVM_Acc_Public) == 0)
            {
                continue;
            }
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

    jboolean Java_java_lang_Class_isInterface(OMElysiaJNIEnv *env, OMElysiaNativeHandle *kls)
    {
        auto k =
            ((OMElysiaKlass *)env->GetLongField(kls, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
        return k->isInstance() && (k->toInstance()->accessFlag & JVM_Acc_Interface);
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

    OMElysiaNativeHandle *Java_java_lang_Class_getDeclaredConstructors0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *kls,
                                                                        bool bl)
    {
        auto kk =
            ((OMElysiaKlass *)env->GetLongField(kls, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
        auto ctkk = env->FindClass("java/lang/reflect/Constructor");
        std::vector<OMElysiaNativeHandle *> hnds;
        for (int i = 0; i < kk->methodCount; i++)
        {
            if (kk->methods[i].isInit())
            {
                auto kns = env->AllocObject(ctkk);

                env->SetObjectField(kns, env->GetFieldID(ctkk, "clazz", "Ljava/lang/Class;"),
                                    env->internal->elysium->executor->recordLocalRef(kk->mirror));
                env->SetIntField(kns, env->GetFieldID(ctkk, "slot", "I"), i);
                env->SetIntField(kns, env->GetFieldID(ctkk, "modifiers", "I"), kk->methods[i].accessFlag);
                env->SetObjectField(kns, env->GetFieldID(ctkk, "signature", "Ljava/lang/String;"),
                                    env->NewStringUTF(kk->methods[i].descriptor));

                auto excarr =
                    env->NewObjectArray(kk->methods[i].exceptionsLength, env->FindClass("java/lang/Class"), nullptr);
                for (int j = 0; j < kk->methods[i].exceptionsLength; j++)
                {
                    env->SetObjectArrayElement(
                        excarr, j,
                        env->internal->elysium->executor->recordLocalRef(kk->methods[i].exceptions[j]->mirror));
                }
                env->SetObjectField(kns, env->GetFieldID(ctkk, "exceptionTypes", "[Ljava/lang/Class;"), excarr);

                throw std::logic_error("not implemented");

                hnds.push_back(kns);
            }
        }

        auto ctarr = env->NewObjectArray(hnds.size(), ctkk, nullptr);
        for (int i = 0; i < hnds.size(); i++)
        {
            env->SetObjectArrayElement(ctarr, i, hnds[i]);
        }

        return ctarr;
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
            {const_cast<char *>("isInterface"), const_cast<char *>("()Z"),
             reinterpret_cast<void *>(Java_java_lang_Class_isInterface)},
            {const_cast<char *>("isAssignableFrom"), const_cast<char *>("(Ljava/lang/Class;)Z"),
             reinterpret_cast<void *>(Java_java_lang_Class_isAssignableFrom)},
            {const_cast<char *>("getDeclaredConstructors0"), const_cast<char *>("(Z)[Ljava/lang/reflect/Constructor;"),
             reinterpret_cast<void *>(Java_java_lang_Class_getDeclaredConstructors0)}};
        env->RegisterNatives(klass, mm, 8);
    }
}
} // namespace openminecraft::vm::elysia::impl
