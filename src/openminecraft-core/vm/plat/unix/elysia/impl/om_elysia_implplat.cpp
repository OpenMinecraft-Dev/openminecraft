#include "openminecraft/vm/elysia/impl/om_elysia_implplat.hpp"

namespace openminecraft::vm::elysia::impl
{
auto getNativeFd(OMElysiaJNIEnv *env, OMElysiaNativeHandle *fdobj) -> uint64_t
{
    return static_cast<uint64_t>(
        env->GetIntField(fdobj, env->GetFieldID(env->FindClass("java/io/FileDescriptor"), "fd", "I")));
}

void setNativeFd(OMElysiaJNIEnv *env, OMElysiaNativeHandle *fdobj, uint64_t fd)
{
    env->SetIntField(fdobj, env->GetFieldID(env->FindClass("java/io/FileDescriptor"), "fd", "I"), fd);
}
} // namespace openminecraft::vm::elysia::impl
