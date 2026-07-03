#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/util/om_util_encoding_utf.hpp"
#include "openminecraft/vm/atomic/om_atomic.hpp"
#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/vm/elysia/om_elysia_descriptor.hpp"
#include "openminecraft/vm/elysia/om_elysia_field.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_meta.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <cstring>

namespace openminecraft::vm::elysia
{
template <typename T> static T getFieldImpl(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID)
{
    return execWithState(InsideVM, [&]() {
        return *reinterpret_cast<T *>(
            env->internal->elysium->oopManager->oopAccessField(handleFetch(obj), fieldID->offset));
    });
}

template <typename T>
static void setFieldImpl(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID, T val)
{
    auto ptr = env->internal->elysium->oopManager->oopAccessField(handleFetch(obj), fieldID->offset);
    *reinterpret_cast<T *>(ptr) = val;
};

template <typename T> static T *getArrayElements(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *isCopy)
{
    return execWithState(InsideVM, [&]() {
        auto result = env->internal->elysium->oopManager->arrAccess<T>(handleFetch(array));
        if (isCopy)
        {
            *isCopy = false;
        }
        atomic::atomic_store(&handleFetch(array)->markword,
                             atomic::atomic_load(&handleFetch(array)->markword) | markFixed);
        return result;
    });
}

template <typename T> static void releaseArrayElements(OMElysiaJNIEnv *env, OMElysiaNativeHandle *arr, T *arrn, jint i)
{
    execWithState(InsideVM, [&]() {
        atomic::atomic_store(&handleFetch(arr)->markword,
                             atomic::atomic_load(&handleFetch(arr)->markword) & ~markFixed);
    });
};

static inline void unlinkMethods(OMElysiaKlass *klass)
{
    for (int i = 0; i < klass->methodCount; ++i)
    {
        auto &m = klass->methods[i];
        if (m.isNative())
        {
            m.code = nullptr;
        }
    }
}

void initBaseInterface(OMElysiaJNIEnv env)
{
    env.internal->GetVersion = [](OMElysiaJNIEnv *) { return JNI_VERSION_1_8; };

    env.internal->FindClass = [](OMElysiaJNIEnv *env, const char *name) {
        return execWithState(InsideVM, [&]() {
            return env->internal->elysium->executor->currentKlassloader()->fetchOrLoadClass(std::string(name));
        });
    };
    env.internal->GetSuperclass = [](OMElysiaJNIEnv *env, OMElysiaKlass *klass) { return klass->superClass; };

    env.internal->Throw = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *t) {
        env->internal->elysium->throwException(handleFetch(t));
        return 0;
    };
    env.internal->ExceptionCheck = [](OMElysiaJNIEnv *env) { return thisThread.metadata->haveException; };
    env.internal->ExceptionOccurred = [](OMElysiaJNIEnv *env) {
        return env->internal->elysium->executor->recordLocalRef(thisThread.metadata->currentException);
    };

    env.internal->AllocObject = [](OMElysiaJNIEnv *env, OMElysiaKlass *klass) {
        return execWithState(InsideVM, [&]() {
            return env->internal->elysium->executor->recordLocalRef(
                env->internal->elysium->oopManager->allocateOop(klass));
        });
    };
    ;

    env.internal->NewObjectA = [](OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                  const OMElysiaNativeValue *args) {
        auto obj = env->AllocObject(clazz);
        env->CallVoidMethodA(obj, methodID, args);
        return obj;
    };

    env.internal->GetObjectClass = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd) {
        return env->internal->elysium->oopManager->oopGetKlass(handleFetch(hnd));
    };

    env.internal->GetMethodID = [](OMElysiaJNIEnv *env, OMElysiaKlass *clazz, const char *name, const char *sig) {
        return clazz->findMethod(name, sig);
    };
    env.internal->CallObjectMethodA = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                                         const OMElysiaNativeValue *args) {
        return execWithState(InsideVM, [&]() {
            auto ar = argCount(methodID->descriptor);
            auto argsCombined = reinterpret_cast<OMElysiaNativeValue *>(
                mem::allocator::tracedCallocElysia(ar + 1, sizeof(OMElysiaNativeValue)));
            argsCombined[0].l = obj;
            std::memcpy(&argsCombined[1], args, ar * sizeof(OMElysiaNativeValue));
            auto hnd =
                env->internal->elysium->executor->recordLocalRef(env->internal->elysium->executor->callObjectFunction(
                    methodID, const_cast<const OMElysiaNativeValue *>(argsCombined)));
            mem::allocator::tracedFreeElysia(argsCombined);
            return hnd;
        });
    };

    env.internal->CallVoidMethodA = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                                       const OMElysiaNativeValue *args) {
        execWithState(InsideVM, [&]() {
            auto ar = argCount(methodID->descriptor);
            auto argsCombined = reinterpret_cast<OMElysiaNativeValue *>(
                mem::allocator::tracedCallocElysia(ar + 1, sizeof(OMElysiaNativeValue)));
            argsCombined[0].l = obj;
            std::memcpy(&argsCombined[1], args, ar * sizeof(OMElysiaNativeValue));
            env->internal->elysium->executor->callVoidFunction(methodID, argsCombined);
            mem::allocator::tracedFreeElysia(argsCombined);
        });
    };

    env.internal->GetFieldID = [](OMElysiaJNIEnv *env, OMElysiaKlass *clazz, const char *name, const char *desc) {
        return clazz->toInstance()->findField(name, desc);
    };
    env.internal->GetObjectField = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID) {
        return execWithState(InsideVM, [&]() {
            return env->internal->elysium->executor->recordLocalRef(
                env->internal->elysium->oopManager->oopAccessPointerField(handleFetch(obj), fieldID->offset));
        });
    };
    env.internal->GetByteField = getFieldImpl<jbyte>;
    env.internal->GetBooleanField = getFieldImpl<jboolean>;
    env.internal->GetCharField = getFieldImpl<jchar>;
    env.internal->GetShortField = getFieldImpl<jshort>;
    env.internal->GetFloatField = getFieldImpl<jfloat>;
    env.internal->GetIntField = getFieldImpl<jint>;
    env.internal->GetDoubleField = getFieldImpl<jdouble>;
    env.internal->GetLongField = getFieldImpl<jlong>;
    env.internal->SetObjectField = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID,
                                      OMElysiaNativeHandle *val) {
        execWithState(InsideVM, [&]() {
            env->internal->elysium->oopManager->oopAccessPointerField(handleFetch(obj), fieldID->offset,
                                                                      handleFetch(val));
        });
    };
    env.internal->SetByteField = setFieldImpl<jbyte>;
    env.internal->SetBooleanField = setFieldImpl<jboolean>;
    env.internal->SetCharField = setFieldImpl<jchar>;
    env.internal->SetShortField = setFieldImpl<jshort>;
    env.internal->SetFloatField = setFieldImpl<jfloat>;
    env.internal->SetIntField = setFieldImpl<jint>;
    env.internal->SetDoubleField = setFieldImpl<jdouble>;
    env.internal->SetLongField = setFieldImpl<jlong>;
    env.internal->SetStaticObjectField = [](OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID,
                                            OMElysiaNativeHandle *value) {
        execWithState(InsideVM, [&]() {
            env->internal->elysium->oopManager->oopAccessPointerStaticField(clazz, fieldID->offset, handleFetch(value));
        });
    };

    env.internal->NewStringUTF = [](OMElysiaJNIEnv *env, const char *string) {
        std::string ss(string);
        return execWithState(InsideVM, [&]() {
            return env->internal->elysium->executor->recordLocalRef(
                env->internal->elysium->oopManager->allocateString(ss));
        });
    };
    env.internal->GetStringUTFChars = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *str, jboolean *isCopy) {
        auto kstr = env->FindClass("java/lang/String");
        auto kfield = env->GetFieldID(kstr, "value", "[C");

        auto arrdata = env->GetObjectField(str, kfield);
        auto data = env->GetCharArrayElements(arrdata, nullptr);
        if (isCopy)
        {
            *isCopy = true;
        }

        return execWithState(InsideVM, [&]() {
            auto len = env->internal->elysium->oopManager->arrLength(handleFetch(arrdata));
            auto s = util::encoding::utf16ToUtf8New(data, len);
            auto result = reinterpret_cast<char *>(mem::allocator::tracedMallocElysia(s.size() + 1));
            std::memcpy(result, s.c_str(), s.size());
            result[s.size()] = '\0';
            return reinterpret_cast<const char *>(result);
        });
    };
    env.internal->ReleaseStringUTFChars = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *str, const char *chars) {
        mem::allocator::tracedFreeElysia(const_cast<char *>(chars));
    };

    env.internal->GetArrayLength = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd) {
        return env->internal->elysium->oopManager->arrLength(handleFetch(hnd));
    };

    env.internal->NewObjectArray = [](OMElysiaJNIEnv *env, jsize len, OMElysiaKlass *klass,
                                      OMElysiaNativeHandle *init) {
        return execWithState(InsideVM, [&]() {
            auto hnd = env->internal->elysium->executor->recordLocalRef(
                env->internal->elysium->oopManager->allocateArr(env->internal->elysium->executor->currentKlassloader()
                                                                    ->fetchOrLoadClass(buildArray(klass->name))
                                                                    ->toArray(),
                                                                len));
            for (int i = 0; i < len; i++)
            {
                env->internal->elysium->oopManager->arrAccessPtr(handleFetch(hnd), i, handleFetch(init));
            }

            return hnd;
        });
    };
    env.internal->GetObjectArrayElement = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize index) {
        return env->internal->elysium->executor->recordLocalRef(
            env->internal->elysium->oopManager->arrAccessPtr(handleFetch(array), index));
    };
    env.internal->SetObjectArrayElement = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize index,
                                             OMElysiaNativeHandle *val) {
        env->internal->elysium->oopManager->arrAccessPtr(handleFetch(array), index, handleFetch(val));
    };
    env.internal->NewCharArray = [](OMElysiaJNIEnv *env, jsize len) {
        return execWithState(InsideVM, [&]() {
            return env->internal->elysium->executor->recordLocalRef(env->internal->elysium->oopManager->allocateArr(
                env->internal->elysium->klassLoader->findClass("[C")->toArray(), len));
        });
    };

    // TODO: copy impl!
    env.internal->GetBooleanArrayElements = getArrayElements;
    env.internal->GetByteArrayElements = getArrayElements;
    env.internal->GetCharArrayElements = getArrayElements;
    env.internal->GetShortArrayElements = getArrayElements;
    env.internal->GetFloatArrayElements = getArrayElements;
    env.internal->GetIntArrayElements = getArrayElements;
    env.internal->GetDoubleArrayElements = getArrayElements;
    env.internal->GetLongArrayElements = getArrayElements;

    env.internal->ReleaseBooleanArrayElements = releaseArrayElements;
    env.internal->ReleaseByteArrayElements = releaseArrayElements;
    env.internal->ReleaseCharArrayElements = releaseArrayElements;
    env.internal->ReleaseShortArrayElements = releaseArrayElements;
    env.internal->ReleaseFloatArrayElements = releaseArrayElements;
    env.internal->ReleaseIntArrayElements = releaseArrayElements;
    env.internal->ReleaseDoubleArrayElements = releaseArrayElements;
    env.internal->ReleaseLongArrayElements = releaseArrayElements;

    env.internal->RegisterNatives = [](OMElysiaJNIEnv *env, OMElysiaKlass *clazz, const OMElysiaNativeMethod *methods,
                                       jint nMethods) {
        execWithState(InsideVM, [&]() {
            if (!clazz->nativeMethods)
            {
                clazz->nativeMethods =
                    (OMElysiaNativeMethod *)mem::allocator::tracedCallocElysia(nMethods, sizeof(OMElysiaNativeMethod));
                clazz->nativeMethodCount = nMethods;
                std::memcpy(clazz->nativeMethods, methods, nMethods * sizeof(OMElysiaNativeMethod));
            }
            else
            {
                auto newdata = (OMElysiaNativeMethod *)mem::allocator::tracedCallocElysia(
                    nMethods + clazz->nativeMethodCount, sizeof(OMElysiaNativeMethod));

                std::memcpy(newdata, methods, nMethods * sizeof(OMElysiaNativeMethod));
                std::memcpy(&newdata[nMethods], clazz->nativeMethods,
                            clazz->nativeMethodCount * sizeof(OMElysiaNativeMethod));
                mem::allocator::tracedFreeElysia(clazz->nativeMethods);
                clazz->nativeMethods = newdata;
                clazz->nativeMethodCount += nMethods;
            }
            unlinkMethods(clazz);
        });
        return 0;
    };
    env.internal->UnregisterNatives = [](OMElysiaJNIEnv *env, OMElysiaKlass *clazz) {
        execWithState(InsideVM, [&]() {
            clazz->nativeMethodCount = 0;
            mem::allocator::tracedFreeElysia(clazz->nativeMethods);
            clazz->nativeMethods = nullptr;
            unlinkMethods(clazz);
        });
        return 0;
    };
    env.internal->MonitorEnter = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd) {
        env->internal->elysium->monitorManager->mutexFetch(handleFetch(hnd));
        return 0;
    };
    env.internal->MonitorExit = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd) {
        env->internal->elysium->monitorManager->mutexRelease(handleFetch(hnd));
        return 0;
    };
}
} // namespace openminecraft::vm::elysia
