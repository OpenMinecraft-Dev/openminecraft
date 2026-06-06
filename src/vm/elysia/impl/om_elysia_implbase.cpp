#include "openminecraft/vm/elysia/impl/om_elysia_implbase.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <stdexcept>

namespace openminecraft::vm::elysia::impl
{
log::OMLogger logger("Elysia Impl Layer");

void Java_java_io_FileDescriptor_initIDs(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
}

void Java_sun_misc_VM_initialize(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
}

OMElysiaNativeHandle *Java_java_lang_System_initProperties(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                           OMElysiaNativeHandle *properties)
{
    return properties;
}

void Java_java_lang_Thread_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
}

void Java_java_lang_System_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    OMElysiaNativeMethod mm[] = {{const_cast<char *>("initProperties"),
                                  const_cast<char *>("(Ljava/util/Properties;)Ljava/util/Properties;"),
                                  reinterpret_cast<void *>(Java_java_lang_System_initProperties)}};
    env->RegisterNatives(klass, mm, 1);
}

static jint Java_java_lang_Object_hashCode(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
{
    return static_cast<jint>(reinterpret_cast<uintptr_t>(hnd->object));
}

void Java_java_lang_Object_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    OMElysiaNativeMethod mm[] = {{const_cast<char *>("hashCode"), const_cast<char *>("()I"),
                                  reinterpret_cast<void *>(Java_java_lang_Object_hashCode)}};
    env->RegisterNatives(klass, mm, 1);
}

OMElysiaNativeHandle *Java_java_lang_Class_getPrimitiveClass(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                                             OMElysiaNativeHandle *name)
{
    auto nn = env->GetStringUTFChars(name, nullptr);
    auto k2 = env->FindClass(nn);
    env->ReleaseStringUTFChars(name, nn);
    auto ff =
        reinterpret_cast<OMElysiaNativeHandle *>(mem::allocator::tracedMallocElysia(sizeof(OMElysiaNativeHandle)));
    ff->next = ff;
    ff->object = k2->mirror;
    return ff;
}

// TODO: check class stat
jboolean Java_java_lang_Class_desiredAssertionStatus0(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaNativeHandle *)
{
    return true;
}

void Java_java_lang_Class_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    OMElysiaNativeMethod mm[] = {
        {const_cast<char *>("getPrimitiveClass"), const_cast<char *>("(Ljava/lang/String;)Ljava/lang/Class;"),
         reinterpret_cast<void *>(Java_java_lang_Class_getPrimitiveClass)},
        {const_cast<char *>("desiredAssertionStatus0"), const_cast<char *>("(Ljava/lang/Class;)Z"),
         reinterpret_cast<void *>(Java_java_lang_Class_desiredAssertionStatus0)}};
    env->RegisterNatives(klass, mm, 2);
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

static jint Java_sun_misc_Unsafe_arrayBaseOffset(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd,
                                                 OMElysiaNativeHandle *klass)
{
    return static_cast<jint>(env->internal->elysium->oopManager->oopArrayHeaderLength());
}

static jint Java_sun_misc_Unsafe_arrayIndexScale(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd,
                                                 OMElysiaNativeHandle *klass)
{
    using namespace binary::hash;

    auto klassKlass = env->FindClass("java/lang/Class");
    auto field = env->GetFieldID(klassKlass, "name", "Ljava/lang/String;");
    auto namestring = env->GetObjectField(klass, field);
    auto clsname = env->GetStringUTFChars(namestring, nullptr);
    jint i = 0;
    switch (hash_compile_time(clsname))
    {
    case "[Z"_hash:
    case "[B"_hash:
        i = 1;
        break;
    case "[C"_hash:
    case "[S"_hash:
        i = 2;
        break;
    case "[I"_hash:
    case "[F"_hash:
        i = 4;
        break;
    case "[J"_hash:
    case "[D"_hash:
        i = 8;
        break;
    default:
        i = env->internal->elysium->mainHeap.ptrLength();
        break;
    }
    env->ReleaseStringUTFChars(namestring, clsname);
    return i;
}

static jint Java_sun_misc_Unsafe_addressSize(OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd)
{
    return sizeof(void *);
}

OMElysiaNativeHandle *Java_sun_reflect_Reflection_getCallerClass(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    auto internal = thisThread.metadata->zero.frame->caller->method->klass->mirror;
    auto ff =
        reinterpret_cast<OMElysiaNativeHandle *>(mem::allocator::tracedMallocElysia(sizeof(OMElysiaNativeHandle)));
    ff->next = ff;
    ff->object = internal;
    return ff;
}

void Java_sun_misc_Unsafe_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    OMElysiaNativeMethod mm[] = {{const_cast<char *>("arrayBaseOffset"), const_cast<char *>("(Ljava/lang/Class;)I"),
                                  reinterpret_cast<void *>(Java_sun_misc_Unsafe_arrayBaseOffset)},
                                 {const_cast<char *>("arrayIndexScale"), const_cast<char *>("(Ljava/lang/Class;)I"),
                                  reinterpret_cast<void *>(Java_sun_misc_Unsafe_arrayIndexScale)},
                                 {const_cast<char *>("addressSize"), const_cast<char *>("()I"),
                                  reinterpret_cast<void *>(Java_sun_misc_Unsafe_addressSize)}};
    env->RegisterNatives(klass, mm, 3);
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
    logger.warn("{}", fmt::ptr(klas));
    throw std::logic_error("not implemented");
}
} // namespace openminecraft::vm::elysia::impl
