#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/specs/classfile/om_classfile.hpp"
#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <cstring>
#include <stdexcept>

namespace openminecraft::vm::elysia::impl
{
extern log::OMLogger logger;
constexpr int MN_Method = 0x10000;
constexpr int MN_Constructor = 0x20000;
constexpr int MN_Field = 0x40000;
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

    static bool resolveField(OMElysiaJNIEnv *env, OMElysiaNativeHandle *memberName, int flags)
    {
        auto type = env->GetObjectField(
            memberName, interface::field(env, "java/lang/invoke/MemberName", "type", "Ljava/lang/Object;"));
        auto nm = env->GetObjectField(
            memberName, interface::field(env, "java/lang/invoke/MemberName", "name", "Ljava/lang/String;"));
        auto k = env->GetObjectField(
            memberName, interface::field(env, "java/lang/invoke/MemberName", "clazz", "Ljava/lang/Class;"));

        auto ffname = env->GetStringUTFChars(nm, nullptr);
        {
            auto fl = env->CallObjectMethodA(
                k, interface::method(env, "java/lang/Class", "getDeclaredFields", "()[Ljava/lang/reflect/Field;"),
                nullptr);
            for (int i = 0; i < env->GetArrayLength(fl); ++i)
            {
                auto ff = env->GetObjectArrayElement(fl, i);
                auto fn = env->GetObjectField(
                    ff, interface::field(env, "java/lang/reflect/Field", "name", "Ljava/lang/String;"));
                auto fnn = env->GetStringUTFChars(fn, nullptr);
                bool equ = std::strcmp(fnn, ffname) == 0;
                env->ReleaseStringUTFChars(fn, fnn);

                if (!equ)
                {
                    continue;
                }

                int fflags = env->GetIntField(ff, interface::field(env, "java/lang/reflect/Field", "modifiers", "I"));
                env->SetIntField(memberName, interface::field(env, "java/lang/invoke/MemberName", "flags", "I"),
                                 flags | (fflags & 0xf));
                env->ReleaseStringUTFChars(nm, ffname);
                return true;
            }
        }
        env->ReleaseStringUTFChars(nm, ffname);
        return false;
    }

    static bool resolveMethod(OMElysiaJNIEnv *env, OMElysiaNativeHandle *memberName, int flags)
    {
        auto type = env->GetObjectField(
            memberName, interface::field(env, "java/lang/invoke/MemberName", "type", "Ljava/lang/Object;"));
        auto nm = env->GetObjectField(
            memberName, interface::field(env, "java/lang/invoke/MemberName", "name", "Ljava/lang/String;"));
        auto k = env->GetObjectField(
            memberName, interface::field(env, "java/lang/invoke/MemberName", "clazz", "Ljava/lang/Class;"));

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
                env->SetIntField(memberName, interface::field(env, "java/lang/invoke/MemberName", "flags", "I"),
                                 flags | (mthflags & 0xf));
                env->ReleaseStringUTFChars(nm, mmname);
                return true;
            end:
                continue;
            }
        }
        logger.warn("{}", mmname);
        env->ReleaseStringUTFChars(nm, mmname);

        return false;
    }

    static OMElysiaNativeHandle *resolve(OMElysiaJNIEnv *env, OMElysiaKlass *, OMElysiaNativeHandle *memberName,
                                         OMElysiaNativeHandle *resolver)
    {
        int flags = env->GetIntField(memberName, interface::field(env, "java/lang/invoke/MemberName", "flags", "I"));
        if (flags & MN_Method)
        {
            if (!resolveMethod(env, memberName, flags))
            {
                throw std::logic_error("resolve method fail!");
            }
        }
        else if (flags & MN_Field)
        {
            if (!resolveField(env, memberName, flags))
            {
                throw std::logic_error("resolve field fail!");
            }
        }
        else
        {
            throw std::logic_error("not a method!");
        }

        return memberName;
    }
    OMElysiaNativeHandle *getMemberVMInfo(OMElysiaJNIEnv *env, OMElysiaKlass *, OMElysiaNativeHandle *memberName)
    {
        jlong idx = -1;
        auto flg = env->GetIntField(memberName, interface::field(env, "java/lang/invoke/MemberName", "flags", "I"));
        auto refkind = (flg >> 24) & 0xf;
        if (refkind == specs::classfile::RefInvokeVirtual || refkind == specs::classfile::RefInvokeInterface ||
            refkind == specs::classfile::RefGetField || refkind == specs::classfile::RefSetField ||
            refkind == specs::classfile::RefGetStatic || refkind == specs::classfile::RefSetStatic)
        {
            idx = 0;
        }

        auto result = env->NewObjectArray(2, env->FindClass("java/lang/Object"), nullptr);
        OMElysiaNativeValue vv[1] = {};
        vv[0].j = idx;
        env->SetObjectArrayElement(result, 0,
                                   env->NewObjectA(env->FindClass("java/lang/Long"),
                                                   interface::method(env, "java/lang/Long", "<init>", "(J)V"), vv));
        if (refkind == specs::classfile::RefGetField || refkind == specs::classfile::RefSetField ||
            refkind == specs::classfile::RefGetStatic || refkind == specs::classfile::RefSetStatic)
        {
            auto l = env->GetObjectField(
                memberName, interface::field(env, "java/lang/invoke/MemberName", "clazz", "Ljava/lang/Class;"));
            env->SetObjectArrayElement(result, 1, l);
        }
        else
        {
            env->SetObjectArrayElement(result, 1, memberName);
        }
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

    static jlong objectFieldOffset(OMElysiaJNIEnv *env, OMElysiaKlass *, OMElysiaNativeHandle *memberName)
    {
        auto nm = env->GetObjectField(
            memberName, interface::field(env, "java/lang/invoke/MemberName", "name", "Ljava/lang/String;"));
        auto k = env->GetObjectField(
            memberName, interface::field(env, "java/lang/invoke/MemberName", "clazz", "Ljava/lang/Class;"));
        auto kk = (OMElysiaKlass *)env->GetLongField(k, interface::field(env, "java/lang/Class", "<ptr>", "J"));
        auto nn = env->GetStringUTFChars(nm, nullptr);
        auto off = kk->toInstance()->findField(nn, nullptr)->offset;
        env->ReleaseStringUTFChars(nm, nn);

        return off + env->internal->elysium->oopManager->oopHeaderLength();
    }

    static jint getMembers(OMElysiaJNIEnv *env, OMElysiaKlass *, OMElysiaNativeHandle *klass,
                           OMElysiaNativeHandle *matchName, OMElysiaNativeHandle *matchSig, int matchFlags,
                           OMElysiaNativeHandle *caller, int skip, OMElysiaNativeHandle *results)
    {
        if ((matchFlags & MN_Method) == 0)
        {
            throw std::logic_error("not implemented!");
        }

        auto fl = env->CallObjectMethodA(
            klass, interface::method(env, "java/lang/Class", "getDeclaredMethods", "()[Ljava/lang/reflect/Method;"),
            nullptr);
        if (matchName || matchSig)
        {
            throw std::logic_error("not implemented!");
        }

        int l = 0;
        for (int i = 0; i < env->GetArrayLength(fl); ++i)
        {
            if (l >= env->GetArrayLength(results))
            {
                break;
            }
            auto m = env->GetObjectArrayElement(fl, i);
            skip--;
            if (skip <= 0)
            {
                OMElysiaNativeValue vv[2];
                vv[0].l = m;
                vv[1].z = false;
                env->SetObjectArrayElement(
                    results, l,
                    env->NewObjectA(env->FindClass("java/lang/invoke/MemberName"),
                                    interface::method(env, "java/lang/invoke/MemberName", "<init>",
                                                      "(Ljava/lang/reflect/Method;Z)V"),
                                    vv));
                ++l;
            }
            else
            {
                continue;
            }
        }

        return l;
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
                {"objectFieldOffset", "(Ljava/lang/invoke/MemberName;)J", objectFieldOffset},
                {"getMembers",
                 "(Ljava/lang/Class;Ljava/lang/String;Ljava/lang/String;ILjava/lang/Class;I[Ljava/lang/invoke/"
                 "MemberName;)I",
                 getMembers},
            });
    }
}
} // namespace openminecraft::vm::elysia::impl
