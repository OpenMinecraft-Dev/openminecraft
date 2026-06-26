#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/vm/elysia/impl/om_elysia_implplat.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_utils.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/os/om_io.hpp"

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    static jint readBytes(OMElysiaJNIEnv *env, OMElysiaNativeHandle *stream, OMElysiaNativeHandle *data, jint off,
                          jint len)
    {
        auto fdobj = env->GetObjectField(
            stream, env->GetFieldID(env->FindClass("java/io/FileOutputStream"), "fd", "Ljava/io/FileDescriptor;"));

        auto a = env->GetByteArrayElements(data, nullptr);
        int l = os::read(getNativeFd(env, fdobj), (uint8_t *)a, off, len);
        env->ReleaseByteArrayElements(data, a, 0);
        return l;
    }
    static jint available0(OMElysiaJNIEnv *env, OMElysiaNativeHandle *stream)
    {
        auto fdobj = env->GetObjectField(
            stream, env->GetFieldID(env->FindClass("java/io/FileOutputStream"), "fd", "Ljava/io/FileDescriptor;"));

        return os::available(getNativeFd(env, fdobj));
    }
    void Java_java_io_FileInputStream_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    {
        interface::registerNativeFuncs(env, klass,
                                       {
                                           {"readBytes", "([BII)I", readBytes},
                                           {"available0", "()I", available0},
                                       });
    }
}
} // namespace openminecraft::vm::elysia::impl
