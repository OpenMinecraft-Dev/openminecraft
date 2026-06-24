#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/os/om_io.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    jint Java_java_io_FileInputStream_readBytes(OMElysiaJNIEnv *env, OMElysiaNativeHandle *stream,
                                                OMElysiaNativeHandle *data, jint off, jint len)
    {
#ifdef OM_PLATFORM_WINDOWS
        auto fdobj = env->GetObjectField(
            stream, env->GetFieldID(env->FindClass("java/io/FileOutputStream"), "fd", "Ljava/io/FileDescriptor;"));
        auto actualfd =
            env->GetLongField(fdobj, env->GetFieldID(env->FindClass("java/io/FileDescriptor"), "handle", "J"));
#else
        auto fdobj = env->GetObjectField(
            stream, env->GetFieldID(env->FindClass("java/io/FileOutputStream"), "fd", "Ljava/io/FileDescriptor;"));
        auto actualfd = env->GetIntField(fdobj, env->GetFieldID(env->FindClass("java/io/FileDescriptor"), "fd", "I"));
#endif

        auto a = env->GetByteArrayElements(data, nullptr);
        int l = os::read(actualfd, (uint8_t *)a, off, len);
        env->ReleaseByteArrayElements(data, a, 0);
        return l;
    }
    void Java_java_io_FileInputStream_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[1] = {{const_cast<char *>("readBytes"), const_cast<char *>("([BII)I"),
                                       reinterpret_cast<void *>(Java_java_io_FileInputStream_readBytes)}};
        env->RegisterNatives(klass, mm, 1);
    }
}
} // namespace openminecraft::vm::elysia::impl
