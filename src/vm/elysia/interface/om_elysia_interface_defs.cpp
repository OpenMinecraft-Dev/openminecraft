#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"

namespace openminecraft::vm::elysia
{
void initBaseInterface(OMElysiaJNIEnv env)
{
    env->GetVersion = [](OMElysiaJNIEnv *) { return JNI_VERSION_1_8; };
}
} // namespace openminecraft::vm::elysia
