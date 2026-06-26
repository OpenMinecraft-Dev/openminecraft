#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
#ifdef OM_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    jlong Java_java_io_FileDescriptor_set(OMElysiaJNIEnv *env, OMElysiaKlass *klass, jint i)
    {
#ifdef OM_PLATFORM_WINDOWS
        return (jlong)_get_osfhandle(i);
#else
        return (jlong)i;
#endif
    }
    void Java_java_io_FileDescriptor_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        interface::registerNativeFuncs(env, klass, {{"set", "(I)J", Java_java_io_FileDescriptor_set}});
    }
}
} // namespace openminecraft::vm::elysia::impl
