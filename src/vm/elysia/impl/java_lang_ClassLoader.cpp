#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <cstring>
#include <stdexcept>

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    static OMElysiaNativeHandle *findBuiltinLib(OMElysiaJNIEnv *env, OMElysiaKlass *, OMElysiaNativeHandle *name)
    {
        return name;
    }
    static OMElysiaNativeHandle *findLoadedClass0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *classloader,
                                                  OMElysiaNativeHandle *name)
    {
        auto ptr =
            env->GetLongField(classloader, env->GetFieldID(env->FindClass("java/lang/ClassLoader"), "<ptr>", "J"));

        if (!ptr)
        {
            return nullptr;
        }
        throw std::logic_error("not implemented");
    }
    static OMElysiaNativeHandle *findBootstrapClass(OMElysiaJNIEnv *env, OMElysiaNativeHandle *cld,
                                                    OMElysiaNativeHandle *name)
    {
        auto nn = env->GetStringUTFChars(name, nullptr);
        std::string c = nn;
        for (auto &ch : c)
        {
            if (ch == '.')
            {
                ch = '/';
            }
        }
        auto kls = env->internal->elysium->klassLoader->fetchOrLoadClass(c);
        env->ReleaseStringUTFChars(name, nn);
        return kls ? createTempHandle(kls->mirror) : nullptr;
    }
    void Java_java_lang_ClassLoader_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        interface::registerNativeFuncs(
            env, klass,
            {
                {"findBuiltinLib", "(Ljava/lang/String;)Ljava/lang/String;", findBuiltinLib},
                {"findLoadedClass0", "(Ljava/lang/String;)Ljava/lang/Class;", findLoadedClass0},
                {"findBootstrapClass", "(Ljava/lang/String;)Ljava/lang/Class;", findBootstrapClass},
            });
    }
}
} // namespace openminecraft::vm::elysia::impl
