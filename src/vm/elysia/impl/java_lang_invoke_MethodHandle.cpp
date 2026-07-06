#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
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
	auto method = (OMElysiaMethod *)env->GetLongField(vmentry, interface::field(env, "java/lang/invoke/MemberName", "<ptr>", "J"));
	logger.info("{} {} {} {}", (void *)method, method->name, method->descriptor, env->GetArrayLength(args));
        logger.warn(
            "{} {}",
            ((OMElysiaKlass *)env->GetLongField(clazz, interface::field(env, "java/lang/Class", "<ptr>", "J")))->name, (void *)handleFetch(vmentry));
        throw std::logic_error("fail");
    }
}
} // namespace openminecraft::vm::elysia::impl
