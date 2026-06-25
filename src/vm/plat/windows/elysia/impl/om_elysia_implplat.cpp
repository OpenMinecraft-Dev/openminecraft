#include "openminecraft/vm/elysia/impl/om_elysia_implplat.hpp"

namespace openminecraft::vm::elysia::impl
{
uint64_t getNativeFd(OMElysiaJNIEnv *env, OMElysiaNativeHandle *fdobj)
{
    return env->GetLongField(fdobj, env->GetFieldID(env->FindClass("java/io/FileDescriptor"), "handle", "J"));
}
} // namespace openminecraft::vm::elysia::impl
