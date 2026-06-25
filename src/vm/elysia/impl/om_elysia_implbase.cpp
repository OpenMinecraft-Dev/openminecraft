#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <atomic>

namespace openminecraft::vm::elysia::impl
{
log::OMLogger logger("Elysia Impl Layer");

OMElysiaNativeHandle *Java_sun_misc_URLClassPath_getLookupCacheURLs(OMElysiaJNIEnv *env, OMElysiaNativeHandle *ucp,
                                                                    OMElysiaNativeHandle *klassloader)
{
    return nullptr;
}

jint Java_java_lang_Runtime_availableProcessors(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance)
{
    return os::fetchAvailableProcessors();
}

// TODO: stub implementation
jint Java_sun_misc_Signal_findSignal(OMElysiaJNIEnv *env, OMElysiaKlass *, OMElysiaNativeHandle *name)
{
    /*using namespace openminecraft::binary::hash;
    jint result = 0;
    auto nn = env->GetStringUTFChars(name, nullptr);
    switch (hash_compile_time(nn))
    {
    case "HUP"_hash:
        result = SIGHUP;
        break;
    case "INT"_hash:
        result = SIGINT;
        break;
    case "TERM"_hash:
        result = SIGTERM;
        break;
    default:
        throw std::logic_error(nn);
    }
    env->ReleaseStringUTFChars(name, nn);

    return result;*/
    return 0;
}

jlong Java_sun_misc_Signal_handle0(OMElysiaJNIEnv *env, OMElysiaKlass *, jint sig, jlong handler)
{
    // return (jlong)signal(sig, (__sighandler_t)handler);
    return 0;
}

void Java_java_lang_ClassLoader$NativeLibrary_load(OMElysiaJNIEnv *env, OMElysiaNativeHandle *lib,
                                                   OMElysiaNativeHandle *name, jboolean)
{
    auto nn = env->GetStringUTFChars(name, nullptr);
    logger.warn("request to load library {}, while elysium doesn't support library loading!", nn);
    env->ReleaseStringUTFChars(name, nn);

    auto kl = env->FindClass("java/lang/ClassLoader$NativeLibrary");
    env->SetBooleanField(lib, env->GetFieldID(kl, "loaded", "Z"), true);
}

jboolean Java_java_util_concurrent_atomic_AtomicLong_VMSupportsCS8(OMElysiaJNIEnv *env, OMElysiaKlass *)
{
    std::atomic_uint64_t l;
    return l.is_lock_free();
}

OMElysiaNativeHandle *Java_java_lang_Throwable_fillInStackTrace(OMElysiaJNIEnv *env, OMElysiaNativeHandle *thr,
                                                                int dummy)
{
    return thr;
}

OMElysiaNativeHandle *Java_java_lang_String_intern(OMElysiaJNIEnv *env, OMElysiaNativeHandle *str)
{
    auto chr = env->GetStringUTFChars(str, nullptr);
    env->internal->elysium->stringPool[binary::hash::hash_compile_time(chr)] = handleFetch(str);
    env->ReleaseStringUTFChars(str, nullptr);
    return str;
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
    if (!thisThread.metadata->zero.frame->caller || !thisThread.metadata->zero.frame->caller->caller)
    {
        return nullptr;
    }
    auto internal = thisThread.metadata->zero.frame->caller->caller->method->klass->mirror;
    return createTempHandle(internal);
}

jint Java_sun_reflect_Reflection_getClassAccessFlags(OMElysiaJNIEnv *env, OMElysiaKlass *, OMElysiaNativeHandle *kls)
{
    return ((OMElysiaKlass *)env->GetLongField(kls, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")))
        ->accessFlag;
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
