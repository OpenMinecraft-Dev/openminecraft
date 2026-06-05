#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/om_elysia_field.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include "openminecraft/vm/encoding/om_encoding_utf.hpp"
#include <cstring>

namespace openminecraft::vm::elysia
{
static log::OMLogger logger("Elysia JNI Layer");

#define enterInterface                                                                                                 \
    auto lastState = thisThread.metadata->state;                                                                       \
    thisThread.switchState(InsideVM);

#define exitInterface thisThread.switchState(lastState);
#define recordResult(op) auto result = op;
#define fetchResult result

static jint interfaceGetVersion(OMElysiaJNIEnv *)
{
    return JNI_VERSION_1_8;
}
static jint interfaceUnregisterNatives(OMElysiaJNIEnv *env, OMElysiaKlass *clazz)
{
    enterInterface;
    clazz->nativeMethodCount = 0;
    mem::allocator::tracedFreeElysia(clazz->nativeMethods);
    clazz->nativeMethods = nullptr;
    exitInterface;
    return 0;
}
static jint interfaceRegisterNatives(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, const OMElysiaNativeMethod *methods,
                                     jint nMethods)
{
    enterInterface;
    if (!clazz->nativeMethods)
    {
        clazz->nativeMethods =
            (OMElysiaNativeMethod *)mem::allocator::tracedCallocElysia(nMethods, sizeof(OMElysiaNativeMethod));
        clazz->nativeMethodCount = nMethods;
        std::memcpy(clazz->nativeMethods, methods, nMethods * sizeof(OMElysiaNativeMethod));
    }
    else
    {
        auto newdata = (OMElysiaNativeMethod *)mem::allocator::tracedCallocElysia(nMethods + clazz->nativeMethodCount,
                                                                                  sizeof(OMElysiaNativeMethod));

        std::memcpy(newdata, methods, nMethods * sizeof(OMElysiaNativeMethod));
        std::memcpy(&newdata[nMethods], clazz->nativeMethods, clazz->nativeMethodCount * sizeof(OMElysiaNativeMethod));
        mem::allocator::tracedFreeElysia(clazz->nativeMethods);
        clazz->nativeMethods = newdata;
        clazz->nativeMethodCount += nMethods;
    }
    exitInterface;
    return 0;
}
static OMElysiaKlass *interfaceFindClass(OMElysiaJNIEnv *env, const char *name)
{
    enterInterface;
    recordResult(env->internal->elysium->klassLoader->fetchOrLoadClass(std::string(name)));
    exitInterface;
    return fetchResult;
}
static OMElysiaNativeHandle *interfaceAllocObject(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    enterInterface;
    recordResult(
        env->internal->elysium->executor->recordLocalRef(env->internal->elysium->oopManager->allocateOop(klass)));
    exitInterface;
    return fetchResult;
};
static OMElysiaKlass *interfaceGetSuperclass(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    return klass->superClass;
}
static OMElysiaField *interfaceGetFieldID(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, const char *name, const char *desc)
{
    return clazz->toInstance()->findField(name, desc);
}
static OMElysiaNativeHandle *interfaceNewCharArray(OMElysiaJNIEnv *env, jsize len)
{
    enterInterface;
    recordResult(env->internal->elysium->executor->recordLocalRef(env->internal->elysium->oopManager->allocateArr(
        env->internal->elysium->klassLoader->findClass("[C")->toArray(), len)));
    exitInterface;
    return fetchResult;
}
static OMElysiaNativeHandle *interfaceNewStringUTF(OMElysiaJNIEnv *env, const char *string)
{
    enterInterface;
    std::string ss(string);
    recordResult(
        env->internal->elysium->executor->recordLocalRef(env->internal->elysium->oopManager->allocateString(ss)));
    exitInterface;
    return fetchResult;
}
static OMElysiaNativeHandle *interfaceGetObjectField(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj,
                                                     OMElysiaField *fieldID)
{
    auto elys = env->internal->elysium;
    enterInterface;
    recordResult(elys->executor->recordLocalRef(elys->oopManager->oopAccessPointerField(obj->object, fieldID->offset)));
    exitInterface;
    return fetchResult;
}
// TODO: copy impl!
static jchar *interfaceGetCharArrayElements(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *isCopy)
{
    enterInterface;
    recordResult(env->internal->elysium->oopManager->arrAccess<jchar>(array->object));
    if (isCopy)
    {
        *isCopy = false;
    }
    array->object->markword |= markFixed;
    exitInterface;
    return fetchResult;
}

static const char *interfaceGetStringUTFChars(OMElysiaJNIEnv *env, OMElysiaNativeHandle *str, jboolean *isCopy)
{
    auto kstr = env->FindClass("java/lang/String");
    auto kfield = env->GetFieldID(kstr, "value", "[C");

    auto arrdata = env->GetObjectField(str, kfield);
    auto data = env->GetCharArrayElements(arrdata, nullptr);
    if (isCopy)
    {
        *isCopy = true;
    }

    enterInterface;
    auto len = env->internal->elysium->oopManager->arrLength(arrdata->object);
    auto s = encoding::utf16ToUtf8New(data, len);
    auto result = reinterpret_cast<char *>(mem::allocator::tracedMallocElysia(s.size() + 1));
    std::memcpy(result, s.c_str(), s.size());
    result[s.size()] = '\0';
    exitInterface;
    return result;
}

static void interfaceReleaseStringUTFChars(OMElysiaJNIEnv *env, OMElysiaNativeHandle *str, const char *chars)
{
    mem::allocator::tracedFreeElysia((void *)(chars));
}

static void interfaceSetObjectField(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID,
                                    OMElysiaNativeHandle *val)
{
    enterInterface;
    env->internal->elysium->oopManager->oopAccessPointerField(obj->object, fieldID->offset, val->object);
    exitInterface;
}

void initBaseInterface(OMElysiaJNIEnv env)
{
    env.internal->GetVersion = interfaceGetVersion;
    env.internal->RegisterNatives = interfaceRegisterNatives;
    env.internal->UnregisterNatives = interfaceUnregisterNatives;
    env.internal->FindClass = interfaceFindClass;
    env.internal->AllocObject = interfaceAllocObject;
    env.internal->GetSuperclass = interfaceGetSuperclass;
    env.internal->GetFieldID = interfaceGetFieldID;
    env.internal->NewCharArray = interfaceNewCharArray;
    env.internal->NewStringUTF = interfaceNewStringUTF;
    env.internal->GetObjectField = interfaceGetObjectField;
    env.internal->GetCharArrayElements = interfaceGetCharArrayElements;
    env.internal->GetStringUTFChars = interfaceGetStringUTFChars;
    env.internal->ReleaseStringUTFChars = interfaceReleaseStringUTFChars;
    env.internal->SetObjectField = interfaceSetObjectField;
}
} // namespace openminecraft::vm::elysia
