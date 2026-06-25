#include "openminecraft/vm/elysia/impl/om_elysia_implplat.hpp"

namespace openminecraft::vm::elysia::impl
{
uint64_t getNativeFd(OMElysiaJNIEnv *env, OMElysiaNativeHandle *fdobj)
{
    return static_cast<uint64_t>(
        env->GetIntField(fdobj, env->GetFieldID(env->FindClass("java/io/FileDescriptor"), "fd", "I")));
}
} // namespace openminecraft::vm::elysia::impl
