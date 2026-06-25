#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <sys/stat.h>

namespace openminecraft::vm::elysia
{
void Java_java_io_UnixFileSystem_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
}
jint Java_java_io_UnixFileSystem_getBooleanAttributes0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *fs,
                                                       OMElysiaNativeHandle *file)
{
    auto flcls = env->FindClass("java/io/File");
    auto pathstr = env->GetObjectField(file, env->GetFieldID(flcls, "path", "Ljava/lang/String;"));

    auto path = env->GetStringUTFChars(pathstr, nullptr);
    jint rv = 0;
    struct stat st;
    if (stat(path, &st) == 0)
    {
        rv |= 0x01;
        if (S_ISREG(st.st_mode))
        {
            rv |= 0x02;
        }
        if (S_ISREG(st.st_mode))
        {
            rv |= 0x04;
        }
    }
    env->ReleaseStringUTFChars(pathstr, path);

    return rv;
}
void OMElysium::registerPlatformNative()
{
    registerNative(Java_java_io_UnixFileSystem_getBooleanAttributes0);
    registerNative(Java_java_io_UnixFileSystem_initIDs);
}
} // namespace openminecraft::vm::elysia
