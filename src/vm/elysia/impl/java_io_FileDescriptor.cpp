#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
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
        HANDLE h = (HANDLE)_get_osfhandle(i);
        return (jlong)h;
#else
        return (jlong)i;
#endif
    }
    void Java_java_io_FileDescriptor_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[1] = {{const_cast<char *>("set"), const_cast<char *>("(I)J"),
                                       reinterpret_cast<void *>(Java_java_io_FileDescriptor_set)}};
        env->RegisterNatives(klass, mm, 1);
    }
}
} // namespace openminecraft::vm::elysia::impl
