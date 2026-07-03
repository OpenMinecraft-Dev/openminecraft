#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    static jint getConstant(OMElysiaJNIEnv *env, OMElysiaKlass *, jint constant)
    {
        return 0;
    }
    void Java_java_lang_invoke_MethodHandleNatives_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        interface::registerNativeFuncs(env, klass, {{"getConstant", "(I)I", getConstant}});
    }
}
} // namespace openminecraft::vm::elysia::impl
