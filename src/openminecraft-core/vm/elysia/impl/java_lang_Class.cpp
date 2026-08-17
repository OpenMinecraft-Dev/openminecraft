#include "openminecraft/specs/classfile/om_classfile.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    static auto getPrimitiveClass(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaNativeHandle *name)
        -> OMElysiaNativeHandle *
    {
        auto nn = env->GetStringUTFChars(name, nullptr);
        auto k2 = env->FindClass(nn);
        env->ReleaseStringUTFChars(name, nn);
        return createTempHandle(k2->mirror);
    }

    static auto desiredAssertionStatus0(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaNativeHandle *klasshnd)
        -> jboolean
    {
        return true;
    }

    static auto forName0(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaNativeHandle *name, jboolean b,
                         OMElysiaNativeHandle *klassloader, OMElysiaNativeHandle *loadcls) -> OMElysiaNativeHandle *
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
                auto kls = kld->fetchOrLoadClass(ss, true);
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

    static auto getDeclaredFields0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *klass, bool publicOnly)
        -> OMElysiaNativeHandle *
    {
        auto kls = env->FindClass("java/lang/Class");
        auto v = reinterpret_cast<OMElysiaKlass *>(env->GetLongField(klass, env->GetFieldID(kls, "<ptr>", "J")))
                     ->toInstance();
        auto fldkls = env->FindClass("java/lang/reflect/Field");
        auto result = env->NewObjectArray(v->fieldCount, fldkls, nullptr);
        for (int i = 0; i < v->fieldCount; i++)
        {
            if (publicOnly && (v->fields[i].accessFlag & JVM_Acc_Public) == 0)
            {
                continue;
            }
            auto kl = fieldDescToType(v->fields[i].desc);
            auto kls = env->FindClass(kl.c_str());

            auto fldobj = env->AllocObject(fldkls);
            env->SetLongField(fldobj, env->GetFieldID(env->FindClass("java/lang/reflect/Field"), "<ptr>", "J"),
                              reinterpret_cast<jlong>(&v->fields[i]));
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

    static auto isPrimitive(OMElysiaJNIEnv *env, OMElysiaNativeHandle *kls) -> jboolean
    {
        return reinterpret_cast<OMElysiaKlass *>(
                   env->GetLongField(kls, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")))
            ->isPrimitive();
    }

    static auto isInterface(OMElysiaJNIEnv *env, OMElysiaNativeHandle *kls) -> jboolean
    {
        auto k = reinterpret_cast<OMElysiaKlass *>(
            env->GetLongField(kls, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
        return k->isInstance() && (k->toInstance()->accessFlag & JVM_Acc_Interface);
    }

    static auto isAssignableFrom(OMElysiaJNIEnv *env, OMElysiaNativeHandle *kls, OMElysiaNativeHandle *klsother)
        -> jboolean
    {
        auto kk = reinterpret_cast<OMElysiaKlass *>(
            env->GetLongField(kls, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
        auto kkother = reinterpret_cast<OMElysiaKlass *>(
            env->GetLongField(klsother, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
        return kkother->inherits(kk);
    }

    static auto getDeclaredConstructors0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *kls, bool publicOnly)
        -> OMElysiaNativeHandle *
    {
        auto kk = reinterpret_cast<OMElysiaKlass *>(
            env->GetLongField(kls, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
        auto ctkk = env->FindClass("java/lang/reflect/Constructor");
        std::vector<OMElysiaNativeHandle *> hnds;
        for (int i = 0; i < kk->methodCount; i++)
        {
            if (kk->methods[i].isInit())
            {
                if (publicOnly && (kk->methods[i].accessFlag & JVM_Acc_Public) == 0)
                {
                    continue;
                }
                auto kns = env->AllocObject(ctkk);

                env->SetLongField(kns, env->GetFieldID(env->FindClass("java/lang/reflect/Constructor"), "<ptr>", "J"),
                                  reinterpret_cast<jlong>(&kk->methods[i]));
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

                auto result = parseSignature(kk->methods[i].descriptor);
                auto argTypearr = env->NewObjectArray(result.first.size(), env->FindClass("java/lang/Class"), nullptr);
                for (int j = 0; j < result.first.size(); j++)
                {
                    env->SetObjectArrayElement(argTypearr, j,
                                               env->internal->elysium->executor->recordLocalRef(
                                                   env->FindClass(signatureToType(result.first[j]).c_str())->mirror));
                }
                env->SetObjectField(kns, env->GetFieldID(ctkk, "parameterTypes", "[Ljava/lang/Class;"), argTypearr);

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

    static auto getModifiers(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd) -> jint
    {
        return reinterpret_cast<OMElysiaKlass *>(
                   env->GetLongField(hnd, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")))
            ->accessFlag;
    }

    static auto getSuperclass(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd) -> OMElysiaNativeHandle *
    {
        auto ls = reinterpret_cast<OMElysiaKlass *>(
                      env->GetLongField(hnd, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")))
                      ->superClass;
        return createTempHandle(ls ? ls->mirror : nullptr);
    }

    static auto isArray(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd) -> jboolean
    {
        auto kls = reinterpret_cast<OMElysiaKlass *>(
            env->GetLongField(hnd, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
        return kls->isArray();
    }

    static auto getComponentType(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd) -> OMElysiaNativeHandle *
    {
        auto kls = reinterpret_cast<OMElysiaKlass *>(
            env->GetLongField(hnd, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
        return kls->isArray() ? createTempHandle(kls->toArray()->lowerDim->mirror) : nullptr;
    }

    static auto getEnclosingMethod0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd) -> OMElysiaNativeHandle *
    {
        auto kls = reinterpret_cast<OMElysiaKlass *>(
            env->GetLongField(hnd, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
        if (!kls->isInstance() || !kls->toInstance()->enclosingMethod)
        {
            return nullptr;
        }
        auto mthd = kls->toInstance()->enclosingMethod;
        std::cout << kls->name << mthd->name << mthd->descriptor << std::endl;
        throw std::logic_error("not implemented!");
    }

    static auto getDeclaringClass0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd) -> OMElysiaNativeHandle *
    {
        auto kls = reinterpret_cast<OMElysiaKlass *>(
            env->GetLongField(hnd, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
        if (!kls->isInstance())
        {
            return nullptr;
        }
        return kls->toInstance()->enclosingKlass ? createTempHandle(kls->toInstance()->enclosingKlass->mirror)
                                                 : nullptr;
    }

    static auto getName0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *klass) -> OMElysiaNativeHandle *
    {
        auto kls = reinterpret_cast<OMElysiaKlass *>(
            env->GetLongField(klass, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));

        auto vv = std::string(kls->name);
        for (auto &ch : vv)
        {
            if (ch == '/')
            {
                ch = '.';
            }

            if (ch == '$')
            {
                break;
            }
        }

        return env->NewStringUTF(vv.c_str());
    }

    static auto getDeclaredMethods0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *klass, jboolean publicOnly)
        -> OMElysiaNativeHandle *
    {
        auto kls = reinterpret_cast<OMElysiaKlass *>(
            env->GetLongField(klass, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
        auto mthk = env->FindClass("java/lang/reflect/Method");

        auto kk = kls->toInstance();
        std::vector<OMElysiaNativeHandle *> hnds;
        for (int i = 0; i < kk->methodCount; i++)
        {
            if (!kk->methods[i].isInit() && !kk->methods->isClinit())
            {
                if (publicOnly && (kk->methods[i].accessFlag & JVM_Acc_Public) == 0)
                {
                    continue;
                }
                auto kns = env->AllocObject(mthk);

                env->SetLongField(kns, env->GetFieldID(mthk, "<ptr>", "J"), reinterpret_cast<jlong>(&kk->methods[i]));
                env->SetObjectField(kns, env->GetFieldID(mthk, "clazz", "Ljava/lang/Class;"),
                                    env->internal->elysium->executor->recordLocalRef(kk->mirror));
                env->SetObjectField(kns, env->GetFieldID(mthk, "name", "Ljava/lang/String;"),
                                    env->NewStringUTF(kk->methods[i].name));
                env->SetIntField(kns, env->GetFieldID(mthk, "slot", "I"), i);
                env->SetIntField(kns, env->GetFieldID(mthk, "modifiers", "I"), kk->methods[i].accessFlag);
                env->SetObjectField(kns, env->GetFieldID(mthk, "signature", "Ljava/lang/String;"),
                                    env->NewStringUTF(kk->methods[i].descriptor));

                auto excarr =
                    env->NewObjectArray(kk->methods[i].exceptionsLength, env->FindClass("java/lang/Class"), nullptr);
                for (int j = 0; j < kk->methods[i].exceptionsLength; j++)
                {
                    env->SetObjectArrayElement(
                        excarr, j,
                        env->internal->elysium->executor->recordLocalRef(kk->methods[i].exceptions[j]->mirror));
                }
                env->SetObjectField(kns, env->GetFieldID(mthk, "exceptionTypes", "[Ljava/lang/Class;"), excarr);

                auto result = parseSignature(kk->methods[i].descriptor);
                auto argTypearr = env->NewObjectArray(result.first.size(), env->FindClass("java/lang/Class"), nullptr);
                for (int j = 0; j < result.first.size(); j++)
                {
                    env->SetObjectArrayElement(argTypearr, j,
                                               env->internal->elysium->executor->recordLocalRef(
                                                   env->FindClass(signatureToType(result.first[j]).c_str())->mirror));
                }
                env->SetObjectField(kns, env->GetFieldID(mthk, "parameterTypes", "[Ljava/lang/Class;"), argTypearr);

                env->SetObjectField(kns, env->GetFieldID(mthk, "returnType", "Ljava/lang/Class;"),
                                    env->internal->elysium->executor->recordLocalRef(
                                        env->FindClass(signatureToType(result.second).c_str())->mirror));
                hnds.push_back(kns);
            }
        }

        auto ctarr = env->NewObjectArray(hnds.size(), mthk, nullptr);
        for (int i = 0; i < hnds.size(); i++)
        {
            env->SetObjectArrayElement(ctarr, i, hnds[i]);
        }

        return ctarr;
    }

    // TODO: impl
    auto getDeclaredClasses0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *klass) -> OMElysiaNativeHandle *
    {
        return env->NewObjectArray(0, env->FindClass("java/lang/Class"), nullptr);
    }

    auto getProtectionDomain0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *klass) -> OMElysiaNativeHandle *
    {
        return nullptr;
    }

    auto isInstance(OMElysiaJNIEnv *env, OMElysiaNativeHandle *klass, OMElysiaNativeHandle *instance) -> jboolean
    {
        auto kls = reinterpret_cast<OMElysiaKlass *>(
            env->GetLongField(klass, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));

        return env->GetObjectClass(instance)->inherits(kls);
    }

    void Java_java_lang_Class_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        interface::registerNativeFuncs(
            env, klass,
            {
                {"getPrimitiveClass", "(Ljava/lang/String;)Ljava/lang/Class;", getPrimitiveClass},
                {"desiredAssertionStatus0", "(Ljava/lang/Class;)Z", desiredAssertionStatus0},
                {"forName0", "(Ljava/lang/String;ZLjava/lang/ClassLoader;Ljava/lang/Class;)Ljava/lang/Class;",
                 forName0},
                {"getDeclaredFields0", "(Z)[Ljava/lang/reflect/Field;", getDeclaredFields0},
                {"isPrimitive", "()Z", isPrimitive},
                {"isInterface", "()Z", isInterface},
                {"isAssignableFrom", "(Ljava/lang/Class;)Z", isAssignableFrom},
                {"getDeclaredConstructors0", "(Z)[Ljava/lang/reflect/Constructor;", getDeclaredConstructors0},
                {"getModifiers", "()I", getModifiers},
                {"getSuperclass", "()Ljava/lang/Class;", getSuperclass},
                {"isArray", "()Z", isArray},
                {"getComponentType", "()Ljava/lang/Class;", getComponentType},
                {"getEnclosingMethod0", "()[Ljava/lang/Object;", getEnclosingMethod0},
                {"getDeclaringClass0", "()Ljava/lang/Class;", getDeclaringClass0},
                {"getName0", "()Ljava/lang/String;", getName0},
                {"getDeclaredMethods0", "(Z)[Ljava/lang/reflect/Method;", getDeclaredMethods0},
                {"getDeclaredClasses0", "()[Ljava/lang/Class;", getDeclaredClasses0},
                {"getProtectionDomain0", "()Ljava/security/ProtectionDomain;", getProtectionDomain0},
                {"isInstance", "(Ljava/lang/Object;)Z", isInstance},
            });
    } // namespace openminecraft::vm::elysia::impl
}
} // namespace openminecraft::vm::elysia::impl
