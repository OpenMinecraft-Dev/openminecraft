#ifndef OM_ELYSIA_IMPLBASE_HPP
#define OM_ELYSIA_IMPLBASE_HPP

#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
namespace openminecraft::vm::elysia
{
class OMElysium;
};

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    void Java_java_lang_System_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass);
    void Java_java_lang_Object_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass);
    void Java_java_lang_Class_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass);
    void Java_java_lang_Thread_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass);
    void Java_java_lang_ClassLoader_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass);

    auto Java_java_lang_Float_floatToRawIntBits(OMElysiaJNIEnv *env, OMElysiaKlass *klass, jfloat f) -> jint;
    auto Java_java_lang_Double_doubleToRawLongBits(OMElysiaJNIEnv *env, OMElysiaKlass *klass, jdouble d) -> jlong;
    auto Java_java_lang_Double_longBitsToDouble(OMElysiaJNIEnv *env, OMElysiaKlass *klass, jlong l) -> jdouble;
    auto Java_java_lang_String_intern(OMElysiaJNIEnv *env, OMElysiaNativeHandle *str) -> OMElysiaNativeHandle *;

    void Java_sun_misc_VM_initialize(OMElysiaJNIEnv *env, OMElysiaKlass *klass);
    void Java_java_io_FileDescriptor_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass);
    void Java_java_io_FileInputStream_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass);
    void Java_java_io_FileOutputStream_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass);

    void Java_sun_misc_Unsafe_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass);
    auto Java_sun_reflect_Reflection_getCallerClass(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
        -> OMElysiaNativeHandle *;
    auto Java_sun_reflect_Reflection_getClassAccessFlags(OMElysiaJNIEnv *env, OMElysiaKlass *,
                                                         OMElysiaNativeHandle *kls) -> jint;

    auto Java_java_security_AccessController_doPrivileged(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                          OMElysiaNativeHandle *action) -> OMElysiaNativeHandle *;
    auto Java_java_security_AccessController_getStackAccessControlContext(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
        -> OMElysiaNativeHandle *;

    auto Java_java_lang_Throwable_fillInStackTrace(OMElysiaJNIEnv *env, OMElysiaNativeHandle *thr, int dummy)
        -> OMElysiaNativeHandle *;
    auto Java_sun_reflect_NativeConstructorAccessorImpl_newInstance0(OMElysiaJNIEnv *env, OMElysiaKlass *,
                                                                     OMElysiaNativeHandle *constructor,
                                                                     OMElysiaNativeHandle *args)
        -> OMElysiaNativeHandle *;
    auto Java_java_util_concurrent_atomic_AtomicLong_VMSupportsCS8(OMElysiaJNIEnv *env, OMElysiaKlass *) -> jboolean;
    void Java_java_lang_ClassLoader$NativeLibrary_load(OMElysiaJNIEnv *env, OMElysiaNativeHandle *,
                                                       OMElysiaNativeHandle *name, jboolean);
    auto Java_sun_misc_Signal_findSignal(OMElysiaJNIEnv *env, OMElysiaKlass *, OMElysiaNativeHandle *name) -> jint;
    auto Java_sun_misc_Signal_handle0(OMElysiaJNIEnv *env, OMElysiaKlass *, jint sig, jlong handler) -> jlong;

    auto Java_java_lang_Runtime_availableProcessors(OMElysiaJNIEnv *env, OMElysiaNativeHandle *instance) -> jint;

    auto Java_sun_misc_URLClassPath_getLookupCacheURLs(OMElysiaJNIEnv *env, OMElysiaNativeHandle *ucp,
                                                       OMElysiaNativeHandle *klassloader) -> OMElysiaNativeHandle *;
    auto Java_java_lang_Float_intBitsToFloat(OMElysiaJNIEnv *env, OMElysiaKlass *, jint i) -> jfloat;
    auto Java_java_lang_reflect_Array_newArray(OMElysiaJNIEnv *env, OMElysiaKlass *, OMElysiaNativeHandle *klass,
                                               jint length) -> OMElysiaNativeHandle *;
    auto Java_java_lang_reflect_Array_getLength(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaNativeHandle *array)
        -> jint;
    void Java_java_lang_invoke_MethodHandleNatives_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass);

    void registerPlatform(OMElysium *);
}
}; // namespace openminecraft::vm::elysia::impl

#endif
