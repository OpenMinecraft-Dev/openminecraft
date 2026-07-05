#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/specs/classfile/om_classfile.hpp"
#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include <stdexcept>

namespace openminecraft::vm::elysia::impl
{
extern log::OMLogger logger;
extern "C"
{
    static jint getConstant(OMElysiaJNIEnv *env, OMElysiaKlass *, jint constant)
    {
        return 0;
    }
    static int getNamedCon(OMElysiaJNIEnv *env, OMElysiaKlass *, jint n, OMElysiaNativeHandle *hnd)
    {
        return 0;
    }
    static OMElysiaNativeHandle *resolve(OMElysiaJNIEnv *env, OMElysiaKlass *, OMElysiaNativeHandle *memberName,
                                         OMElysiaNativeHandle *resolver)
    {
        int flags = env->GetIntField(memberName, interface::field(env, "java/lang/invoke/MemberName", "flags", "I"));
        auto type = env->GetObjectField(
            memberName, interface::field(env, "java/lang/invoke/MemberName", "type", "Ljava/lang/Object;"));
        auto nm = env->GetObjectField(
            memberName, interface::field(env, "java/lang/invoke/MemberName", "name", "Ljava/lang/String;"));
        auto k = env->GetObjectField(
            memberName, interface::field(env, "java/lang/invoke/MemberName", "clazz", "Ljava/lang/Class;"));
        if ((flags & 0x10000) == 0)
        {
            throw std::logic_error("not a method!");
        }

        auto mmname = env->GetStringUTFChars(nm, nullptr);
        {
            auto rettype = env->GetObjectField(
                type, interface::field(env, "java/lang/invoke/MethodType", "rtype", "Ljava/lang/Class;"));
            auto ptypes = env->GetObjectField(
                type, interface::field(env, "java/lang/invoke/MethodType", "ptypes", "[Ljava/lang/Class;"));

            auto ml = env->CallObjectMethodA(
                k, interface::method(env, "java/lang/Class", "getDeclaredMethods", "()[Ljava/lang/reflect/Method;"),
                nullptr);

            auto mn = env->GetArrayLength(ml);
            for (int i = 0; i < mn; ++i)
            {
                auto mth = env->GetObjectArrayElement(ml, i);
                auto mthflags =
                    env->GetIntField(mth, interface::field(env, "java/lang/reflect/Method", "modifiers", "I"));
                auto mname = env->GetObjectField(
                    mth, interface::field(env, "java/lang/reflect/Method", "name", "Ljava/lang/String;"));

                bool equ;
                auto mmn = env->GetStringUTFChars(mname, nullptr);
                equ = (std::strcmp(mmname, mmn) == 0);
                env->ReleaseStringUTFChars(mname, mmn);

                if (!equ)
                {
                    continue;
                }

                // TODO: skipped!
                /*auto mrettype = env->GetObjectField(
                    mth, interface::field(env, "java/lang/reflect/Method", "returnType", "Ljava/lang/Class;"));

                if (handleFetch(mrettype) != handleFetch(rettype))
                {
                    continue;
                }

                auto mptypes = env->GetObjectField(
                    mth, interface::field(env, "java/lang/reflect/Method", "parameterTypes", "[Ljava/lang/Class;"));
                int length = env->GetArrayLength(mptypes);
                if (length != env->GetArrayLength(ptypes))
                {
                    continue;
                }

                for (int i = 0; i < length; ++i)
                {
                    auto t1 = env->GetObjectArrayElement(mptypes, i);
                    auto t2 = env->GetObjectArrayElement(ptypes, i);

                    if (handleFetch(t1) != handleFetch(t2))
                    {
                        goto end;
                    }
                }*/

            success:
                env->SetObjectField(
                    memberName,
                    interface::field(env, "java/lang/invoke/MemberName", "resolution", "Ljava/lang/Object;"), mth);
                env->SetIntField(memberName, interface::field(env, "java/lang/invoke/MemberName", "flags", "I"),
                                 flags | mthflags);
                env->ReleaseStringUTFChars(nm, mmname);
                return memberName;
            end:
                continue;
            }
        }
        logger.warn("{}", mmname);
        env->ReleaseStringUTFChars(nm, mmname);

        throw std::logic_error("fail");
    }
    OMElysiaNativeHandle *getMemberVMInfo(OMElysiaJNIEnv *env, OMElysiaKlass *, OMElysiaNativeHandle *memberName)
    {
        jlong idx = -1;
        auto flg = env->GetIntField(memberName, interface::field(env, "java/lang/invoke/MemberName", "flags", "I"));
        auto refkind = (flg >> 24) & 0xf;
        if (refkind == specs::classfile::RefInvokeVirtual || refkind == specs::classfile::RefInvokeInterface)
        {
            idx = 0;
        }

        auto result = env->NewObjectArray(2, env->FindClass("java/lang/Object"), nullptr);
        OMElysiaNativeValue vv[1] = {};
        vv[0].j = idx;
        env->SetObjectArrayElement(result, 0,
                                   env->NewObjectA(env->FindClass("java/lang/Long"),
                                                   interface::method(env, "java/lang/Long", "<init>", "(J)V"), vv));
        env->SetObjectArrayElement(result, 1, memberName);
        return result;
    }

    static void init(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaNativeHandle *memberName,
                     OMElysiaNativeHandle *obj)
    {
        auto ll = env->GetObjectClass(obj);
        if (std::strcmp("java/lang/reflect/Method", ll->name))
        {
            throw std::logic_error("not implemented");
        }

        auto kls =
            env->GetObjectField(obj, interface::field(env, "java/lang/reflect/Method", "clazz", "Ljava/lang/Class;"));

        env->SetObjectField(memberName,
                            interface::field(env, "java/lang/invoke/MemberName", "clazz", "Ljava/lang/Class;"), kls);
        env->SetObjectField(
            memberName, interface::field(env, "java/lang/invoke/MemberName", "name", "Ljava/lang/String;"),
            env->GetObjectField(obj, interface::field(env, "java/lang/reflect/Method", "name", "Ljava/lang/String;")));

        auto flg = env->GetIntField(obj, interface::field(env, "java/lang/reflect/Method", "modifiers", "I"));
        if (flg & JVM_Acc_Static)
        {
            flg |= (static_cast<int>(specs::classfile::RefInvokeStatic) << 24);
        }
        else if (flg & JVM_Acc_Private)
        {
            flg |= (static_cast<int>(specs::classfile::RefInvokeSepcial) << 24);
        }
        else if ((((OMElysiaKlass *)env->GetLongField(kls, interface::field(env, "java/lang/Class", "<ptr>", "J")))
                      ->accessFlag &
                  JVM_Acc_Interface) &&
                 (flg & JVM_Acc_Abstract))
        {
            flg |= (static_cast<int>(specs::classfile::RefInvokeInterface) << 24);
        }
        else
        {
            flg |= (static_cast<int>(specs::classfile::RefInvokeVirtual) << 24);
        }
        env->SetIntField(memberName, interface::field(env, "java/lang/invoke/MemberName", "flags", "I"), flg | 0x10000);

        auto rettype = env->GetObjectField(
            obj, interface::field(env, "java/lang/reflect/Method", "returnType", "Ljava/lang/Class;"));
        auto partypes = env->GetObjectField(
            obj, interface::field(env, "java/lang/reflect/Method", "parameterTypes", "[Ljava/lang/Class;"));

        OMElysiaNativeValue vv[3];
        vv[0].l = rettype;
        vv[1].l = partypes;
        vv[2].z = false;
        auto mt = env->CallStaticObjectMethodA(
            env->FindClass("java/lang/invoke/MethodType"),
            interface::staticMethod(env, "java/lang/invoke/MethodType", "makeImpl",
                                    "(Ljava/lang/Class;[Ljava/lang/Class;Z)Ljava/lang/invoke/MethodType;"),
            vv);
        env->SetObjectField(memberName,
                            interface::field(env, "java/lang/invoke/MemberName", "type", "Ljava/lang/Object;"), mt);
    }
    void Java_java_lang_invoke_MethodHandleNatives_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        interface::registerNativeFuncs(
            env, klass,
            {
                {"getConstant", "(I)I", getConstant},
                {"getNamedCon", "(I[Ljava/lang/Object;)I", getNamedCon},
                {"resolve", "(Ljava/lang/invoke/MemberName;Ljava/lang/Class;)Ljava/lang/invoke/MemberName;", resolve},
                {"getMemberVMInfo", "(Ljava/lang/invoke/MemberName;)Ljava/lang/Object;", getMemberVMInfo},
                {"init", "(Ljava/lang/invoke/MemberName;Ljava/lang/Object;)V", init},
            });
    }
}
} // namespace openminecraft::vm::elysia::impl
