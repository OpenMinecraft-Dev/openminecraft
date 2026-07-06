#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include <stdexcept>

namespace openminecraft::vm::elysia::impl
{
extern log::OMLogger logger;
extern "C"
{
    OMElysiaNativeHandle *Java_java_lang_invoke_MethodHandle_invoke(OMElysiaJNIEnv *env, OMElysiaNativeHandle *handle,
                                                                    OMElysiaNativeHandle *args)
    {
        auto form = env->GetObjectField(
            handle, interface::field(env, "java/lang/invoke/MethodHandle", "form", "Ljava/lang/invoke/LambdaForm;"));
        auto vmentry = env->GetObjectField(
            form, interface::field(env, "java/lang/invoke/LambdaForm", "vmentry", "Ljava/lang/invoke/MemberName;"));
        auto clazz = env->GetObjectField(
            vmentry, interface::field(env, "java/lang/invoke/MemberName", "clazz", "Ljava/lang/Class;"));
        auto name = env->GetObjectField(
            vmentry, interface::field(env, "java/lang/invoke/MemberName", "name", "Ljava/lang/String;"));
        auto method = (OMElysiaMethod *)env->GetLongField(
            vmentry, interface::field(env, "java/lang/invoke/MemberName", "<ptr>", "J"));
        auto ll = (OMElysiaNativeValue *)malloc(sizeof(OMElysiaNativeValue) * (env->GetArrayLength(args) + 1));
        ll[0].l = handle;
        for (int i = 0; i < env->GetArrayLength(args); ++i)
        {
            ll[i + 1].l = env->GetObjectArrayElement(args, i);
        }
        auto r = env->CallStaticObjectMethodA(
            (OMElysiaKlass *)env->GetLongField(clazz, interface::field(env, "java/lang/Class", "<ptr>", "J")), method,
            ll);
        free(ll);
        return r;
    }
}
} // namespace openminecraft::vm::elysia::impl
