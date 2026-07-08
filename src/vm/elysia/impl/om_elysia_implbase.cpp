#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <atomic>

namespace openminecraft::vm::elysia::impl
{
log::OMLogger logger("Elysia Impl Layer");

auto Java_java_lang_reflect_Array_getLength(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaNativeHandle *array)
    -> jint
{
    return env->GetArrayLength(array);
}

auto Java_java_lang_reflect_Array_newArray(OMElysiaJNIEnv *env, OMElysiaKlass *, OMElysiaNativeHandle *klass,
                                           jint length) -> OMElysiaNativeHandle *
{
    auto kls =
        ((OMElysiaKlass *)env->GetLongField(klass, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")));
    auto arrcls = env->FindClass(buildArray(kls->name).c_str());
    return createTempHandle(env->internal->elysium->oopManager->allocateArr(arrcls->toArray(), length));
}

auto Java_sun_misc_URLClassPath_getLookupCacheURLs(OMElysiaJNIEnv *env, OMElysiaNativeHandle *ucp,
                                                   OMElysiaNativeHandle *klassloader) -> OMElysiaNativeHandle *
{
    return nullptr;
}

auto Java_java_lang_Runtime_availableProcessors(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance) -> jint
{
    return os::fetchAvailableProcessors();
}

// TODO: stub implementation
auto Java_sun_misc_Signal_findSignal(OMElysiaJNIEnv *env, OMElysiaKlass *, OMElysiaNativeHandle *name) -> jint
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

auto Java_sun_misc_Signal_handle0(OMElysiaJNIEnv *env, OMElysiaKlass *, jint sig, jlong handler) -> jlong
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

auto Java_java_util_concurrent_atomic_AtomicLong_VMSupportsCS8(OMElysiaJNIEnv *env, OMElysiaKlass *) -> jboolean
{
    std::atomic_uint64_t l;
    return l.is_lock_free();
}

auto Java_java_lang_Throwable_fillInStackTrace(OMElysiaJNIEnv *env, OMElysiaNativeHandle *thr, int dummy)
    -> OMElysiaNativeHandle *
{
    return thr;
}

auto Java_java_lang_String_intern(OMElysiaJNIEnv *env, OMElysiaNativeHandle *str) -> OMElysiaNativeHandle *
{
    auto chr = env->GetStringUTFChars(str, nullptr);
    env->internal->elysium->stringPool[binary::hash::hash_compile_time(chr)] = handleFetch(str);
    env->ReleaseStringUTFChars(str, nullptr);
    return str;
}

void Java_sun_misc_VM_initialize(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
}

auto Java_java_lang_Float_floatToRawIntBits(OMElysiaJNIEnv *env, OMElysiaKlass *klass, jfloat f) -> jint
{
    return *reinterpret_cast<jint *>(&f);
}

auto Java_java_lang_Double_doubleToRawLongBits(OMElysiaJNIEnv *env, OMElysiaKlass *klass, jdouble d) -> jlong
{
    return *reinterpret_cast<jlong *>(&d);
}

auto Java_java_lang_Double_longBitsToDouble(OMElysiaJNIEnv *env, OMElysiaKlass *klass, jlong l) -> jdouble
{
    return *reinterpret_cast<jdouble *>(&l);
}

// TODO: make this portable for future executors
auto Java_sun_reflect_Reflection_getCallerClass(OMElysiaJNIEnv *env, OMElysiaKlass *klass) -> OMElysiaNativeHandle *
{
    if (!thisThread.metadata->zero.frame->caller || !thisThread.metadata->zero.frame->caller->caller)
    {
        return nullptr;
    }
    auto internal = thisThread.metadata->zero.frame->caller->caller->method->klass->mirror;
    return createTempHandle(internal);
}

auto Java_sun_reflect_Reflection_getClassAccessFlags(OMElysiaJNIEnv *env, OMElysiaKlass *, OMElysiaNativeHandle *kls)
    -> jint
{
    return reinterpret_cast<OMElysiaKlass *>(
               env->GetLongField(kls, env->GetFieldID(env->FindClass("java/lang/Class"), "<ptr>", "J")))
        ->accessFlag;
}

auto Java_java_security_AccessController_doPrivileged(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                      OMElysiaNativeHandle *action) -> OMElysiaNativeHandle *
{
    auto klas = env->FindClass("java/security/PrivilegedExceptionAction");
    auto mthd = env->GetMethodID(klas, "run", "()Ljava/lang/Object;");
    return env->CallObjectMethodA(action, mthd, nullptr);
}

auto Java_java_security_AccessController_getStackAccessControlContext(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
    -> OMElysiaNativeHandle *
{
    return nullptr;
}

auto Java_java_lang_Float_intBitsToFloat(OMElysiaJNIEnv *env, OMElysiaKlass *, jint i) -> jfloat
{
    return *reinterpret_cast<jfloat *>(&i);
}
} // namespace openminecraft::vm::elysia::impl
