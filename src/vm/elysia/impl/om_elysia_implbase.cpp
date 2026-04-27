#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"

namespace openminecraft::vm::elysia::impl
{
void Java_java_lang_System_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    OMElysiaNativeMethod mm[1] = {{"initProperties", "(Ljava/util/Properties;)V", (void *)0x123}};
    (*env)->RegisterNatives(env, klass, mm, 1);
}
} // namespace openminecraft::vm::elysia::impl
