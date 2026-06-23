#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include <iostream>

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    OMElysiaNativeHandle *Java_sun_reflect_NativeConstructorAccessorImpl_newInstance0(OMElysiaJNIEnv *env,
                                                                                      OMElysiaKlass *,
                                                                                      OMElysiaNativeHandle *constructor,
                                                                                      OMElysiaNativeHandle *args)
    {
        auto cst = env->FindClass("java/lang/reflect/Constructor");
        auto nm = env->GetObjectField(constructor, env->GetFieldID(cst, "signature", "Ljava/lang/String;"));
        auto strm = env->GetStringUTFChars(nm, nullptr);

        auto kls = ((OMElysiaKlass *)env->GetLongField(
            env->GetObjectField(constructor, env->GetFieldID(cst, "clazz", "Ljava/lang/Class;")),
            env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));

        auto method = env->GetMethodID(kls, "<init>", strm);
        env->ReleaseStringUTFChars(nm, strm);

        if (std::strcmp("()V", method->descriptor) == 0)
        {
            return env->NewObjectA(kls, method, nullptr);
        }
        else
        {
            throw std::logic_error("not impl!");
        }
    }
}
} // namespace openminecraft::vm::elysia::impl
