#include "openminecraft/vm/elysia/impl/om_elysia_implplat.hpp"

namespace openminecraft::vm::elysia::impl
{
uint64_t getNativeFd(OMElysiaJNIEnv *env, OMElysiaNativeHandle *fdobj)
{
    return env->GetLongField(fdobj, env->GetFieldID(env->FindClass("java/io/FileDescriptor"), "handle", "J"));
}

void setNativeFd(OMElysiaJNIEnv *env, OMElysiaNativeHandle *fd, uint64_t n)
{
    env->SetLongField(fd, env->GetFieldID(env->FindClass("java/io/FileDescriptor"), "handle", "J"), n);
}
} // namespace openminecraft::vm::elysia::impl
