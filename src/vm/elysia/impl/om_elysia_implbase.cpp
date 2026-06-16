#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"

namespace openminecraft::vm::elysia::impl
{
log::OMLogger logger("Elysia Impl Layer");

OMElysiaNativeHandle *Java_java_lang_String_intern(OMElysiaJNIEnv *env, OMElysiaNativeHandle *str)
{
    auto chr = env->GetStringUTFChars(str, nullptr);
    env->internal->elysium->stringPool[binary::hash::hash_compile_time(chr)] = handleFetch(str);
    env->ReleaseStringUTFChars(str, nullptr);
    return str;
}

void Java_java_io_FileDescriptor_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
}

void Java_sun_misc_VM_initialize(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
}

jint Java_java_lang_Float_floatToRawIntBits(OMElysiaJNIEnv *env, OMElysiaKlass *klass, jfloat f)
{
    return *reinterpret_cast<jint *>(&f);
}

jlong Java_java_lang_Double_doubleToRawLongBits(OMElysiaJNIEnv *env, OMElysiaKlass *klass, jdouble d)
{
    return *reinterpret_cast<jlong *>(&d);
}

jdouble Java_java_lang_Double_longBitsToDouble(OMElysiaJNIEnv *env, OMElysiaKlass *klass, jlong l)
{
    return *reinterpret_cast<jdouble *>(&l);
}

// TODO: make this portable for future executors
OMElysiaNativeHandle *Java_sun_reflect_Reflection_getCallerClass(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    if (!thisThread.metadata->zero.frame->caller)
    {
        return nullptr;
    }
    auto internal = thisThread.metadata->zero.frame->caller->method->klass->mirror;
    return createTempHandle(internal);
}

void Java_java_io_FileInputStream_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
}

void Java_java_io_FileOutputStream_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
}

OMElysiaNativeHandle *Java_java_security_AccessController_doPrivileged(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                                       OMElysiaNativeHandle *action)
{
    auto klas = env->FindClass("java/security/PrivilegedExceptionAction");
    auto mthd = env->GetMethodID(klas, "run", "()Ljava/lang/Object;");
    return env->CallObjectMethodA(action, mthd, nullptr);
}

OMElysiaNativeHandle *Java_java_security_AccessController_getStackAccessControlContext(OMElysiaJNIEnv *env,
                                                                                       OMElysiaKlass *klass)
{
    return nullptr;
}
} // namespace openminecraft::vm::elysia::impl
