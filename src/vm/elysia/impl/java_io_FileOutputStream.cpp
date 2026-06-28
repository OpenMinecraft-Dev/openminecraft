#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/impl/om_elysia_implplat.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
#include "openminecraft/vm/os/om_io.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    static void writeBytes(OMElysiaJNIEnv *env, OMElysiaNativeHandle *stream, OMElysiaNativeHandle *data, jint off,
                           jint len, jboolean append)
    {
        auto fdobj = env->GetObjectField(
            stream, env->GetFieldID(env->FindClass("java/io/FileOutputStream"), "fd", "Ljava/io/FileDescriptor;"));

        auto a = env->GetByteArrayElements(data, nullptr);
        os::write(getNativeFd(env, fdobj), (uint8_t *)a, off, len, append);
        env->ReleaseByteArrayElements(data, a, 0);
    }
    static void open0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *stream, OMElysiaNativeHandle *path, jboolean append)
    {
        auto fdobj = env->GetObjectField(
            stream, env->GetFieldID(env->FindClass("java/io/FileOutputStream"), "fd", "Ljava/io/FileDescriptor;"));

        auto p = env->GetStringUTFChars(path, nullptr);
        setNativeFd(env, fdobj, os::open(p, append));
        env->ReleaseStringUTFChars(path, p);
    }
    static void close0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *stream)
    {
        auto fdobj = env->GetObjectField(
            stream, env->GetFieldID(env->FindClass("java/io/FileOutputStream"), "fd", "Ljava/io/FileDescriptor;"));
        os::close(getNativeFd(env, fdobj));
    }
    void Java_java_io_FileOutputStream_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        interface::registerNativeFuncs(env, klass,
                                       {
                                           {"writeBytes", "([BIIZ)V", writeBytes},
                                           {"open0", "(Ljava/lang/String;Z)V", open0},
                                           {"close0", "()V", close0},
                                       });
    }
}
} // namespace openminecraft::vm::elysia::impl
