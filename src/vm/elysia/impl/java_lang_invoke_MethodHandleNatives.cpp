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
                auto mname = env->GetObjectField(
                    mth, interface::field(env, "java/lang/reflect/Method", "name", "Ljava/lang/String;"));

                bool equ;
                auto mmn = env->GetStringUTFChars(mname, nullptr);
                equ = std::strcmp(mmname, mmn) == 0;
                env->ReleaseStringUTFChars(mname, mmn);

                if (!equ)
                {
                    continue;
                }

                auto mrettype = env->GetObjectField(
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
                }

                env->SetObjectField(
                    memberName,
                    interface::field(env, "java/lang/invoke/MemberName", "resolution", "Ljava/lang/Object;"), mth);
                env->ReleaseStringUTFChars(nm, mmname);
                return memberName;
            end:
                continue;
            }
        }

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
    void Java_java_lang_invoke_MethodHandleNatives_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        interface::registerNativeFuncs(
            env, klass,
            {
                {"getConstant", "(I)I", getConstant},
                {"getNamedCon", "(I[Ljava/lang/Object;)I", getNamedCon},
                {"resolve", "(Ljava/lang/invoke/MemberName;Ljava/lang/Class;)Ljava/lang/invoke/MemberName;", resolve},
                {"getMemberVMInfo", "(Ljava/lang/invoke/MemberName;)Ljava/lang/Object;", getMemberVMInfo},
            });
    }
}
} // namespace openminecraft::vm::elysia::impl
