#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/om_elysia_field.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
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
beg:
    auto klass = env->internal->world->klassLoader->findClass(std::string(name));
    if (!klass)
    {
        env->internal->world->klassLoader->loadClass(std::string(name));
        goto beg;
    }
    else
    {
        exitInterface;
        return klass;
    }
}
static OMElysiaNativeHandle *interfaceAllocObject(OMElysiaJNIEnv *env, OMElysiaKlass *klass)
{
    enterInterface;
    recordResult(env->internal->world->executor->recordLocalRef(env->internal->world->oopManager->allocateOop(klass)));
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
    recordResult(env->internal->world->executor->recordLocalRef(env->internal->world->oopManager->allocateArr(
        env->internal->world->klassLoader->findClass("[C")->toArray(), len)));
    exitInterface;
    return fetchResult;
}
static OMElysiaNativeHandle *interfaceNewStringUTF(OMElysiaJNIEnv *env, const char *string)
{
    enterInterface;
    std::string ss(string);
    recordResult(env->internal->world->executor->recordLocalRef(env->internal->world->oopManager->allocateString(ss)));
    exitInterface;
    return fetchResult;
}
static OMElysiaNativeHandle *interfaceGetObjectField(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj,
                                                     OMElysiaField *fieldID)
{
    auto world = env->internal->world;
    enterInterface;
    recordResult(
        world->executor->recordLocalRef(world->oopManager->oopAccessPointerField(obj->object, fieldID->offset)));
    exitInterface;
    return fetchResult;
}
// TODO: copy impl!
static jchar *interfaceGetCharArrayElements(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *isCopy)
{
    enterInterface;
    recordResult(
        env->internal->world->oopManager->arrAccess<jchar>(reinterpret_cast<OMElysiaArrayOop *>(array->object)));
    if (isCopy)
    {
        *isCopy = false;
    }
    array->object->markword |= markFixed;
    exitInterface;
    return fetchResult;
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
}
} // namespace openminecraft::vm::elysia
