#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/os/om_io.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    void Java_java_io_FileOutputStream_writeBytes(OMElysiaJNIEnv *env, OMElysiaNativeHandle *stream,
                                                  OMElysiaNativeHandle *data, jint off, jint len, jboolean append)
    {
        auto fdobj = env->GetObjectField(
            stream, env->GetFieldID(env->FindClass("java/io/FileOutputStream"), "fd", "Ljava/io/FileDescriptor;"));
        auto actualfd = env->GetIntField(fdobj, env->GetFieldID(env->FindClass("java/io/FileDescriptor"), "fd", "I"));

        auto a = env->GetByteArrayElements(data, nullptr);
        os::write(actualfd, (uint8_t *)a, off, len, append);
        env->ReleaseByteArrayElements(data, a, 0);
    }
    void Java_java_io_FileOutputStream_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        OMElysiaNativeMethod mm[1] = {{const_cast<char *>("writeBytes"), const_cast<char *>("([BIIZ)V"),
                                       reinterpret_cast<void *>(Java_java_io_FileOutputStream_writeBytes)}};
        env->RegisterNatives(klass, mm, 1);
    }
}
} // namespace openminecraft::vm::elysia::impl
