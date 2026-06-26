#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
#include "openminecraft/vm/os/om_io.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    static jlong set(OMElysiaJNIEnv *env, OMElysiaKlass *klass, jint i)
    {
        return os::convertHandle(i);
    }
    void Java_java_io_FileDescriptor_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        interface::registerNativeFuncs(env, klass, {{"set", "(I)J", set}});
    }
}
} // namespace openminecraft::vm::elysia::impl
