#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
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
#include "openminecraft/vm/encoding/om_encoding_utf.hpp"
#include <cstring>

namespace openminecraft::vm::elysia
{
void initBaseInterface(OMElysiaJNIEnv env)
{
    env.internal->GetVersion = [](OMElysiaJNIEnv *) { return JNI_VERSION_1_8; };

    env.internal->FindClass = [](OMElysiaJNIEnv *env, const char *name) {
        OMElysiaKlass *klass;
        execWithState(InsideVM, [&]() {
            klass = env->internal->elysium->executor->currentKlassloader()->fetchOrLoadClass(std::string(name));
        });
        return klass;
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
        OMElysiaNativeHandle *hnd;
        execWithState(InsideVM, [&]() {
            hnd = env->internal->elysium->executor->recordLocalRef(
                env->internal->elysium->oopManager->allocateOop(klass));
        });
        return hnd;
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
        OMElysiaNativeHandle *hnd;
        execWithState(InsideVM, [&]() {
            auto ar = argCount(methodID->descriptor);
            auto argsCombined = reinterpret_cast<OMElysiaNativeValue *>(
                mem::allocator::tracedCallocElysia(ar + 1, sizeof(OMElysiaNativeValue)));
            argsCombined[0].l = obj;
            std::memcpy(&argsCombined[1], args, ar * sizeof(OMElysiaNativeValue));
            hnd = env->internal->elysium->executor->recordLocalRef(env->internal->elysium->executor->callObjectFunction(
                methodID, const_cast<const OMElysiaNativeValue *>(argsCombined)));
            mem::allocator::tracedFreeElysia(argsCombined);
        });
        return hnd;
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
        auto elys = env->internal->elysium;
        OMElysiaNativeHandle *hnd;
        execWithState(InsideVM, [&]() {
            hnd = elys->executor->recordLocalRef(
                elys->oopManager->oopAccessPointerField(handleFetch(obj), fieldID->offset));
        });
        return hnd;
    };
    env.internal->GetIntField = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID) {
        auto elys = env->internal->elysium;
        jint val;
        execWithState(InsideVM, [&]() {
            val = *reinterpret_cast<jint *>(elys->oopManager->oopAccessField(handleFetch(obj), fieldID->offset));
        });
        return val;
    };
    env.internal->GetLongField = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID) {
        auto elys = env->internal->elysium;
        jlong val;
        execWithState(InsideVM, [&]() {
            val = *reinterpret_cast<jlong *>(elys->oopManager->oopAccessField(handleFetch(obj), fieldID->offset));
        });
        return val;
    };
    env.internal->SetObjectField = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID,
                                      OMElysiaNativeHandle *val) {
        execWithState(InsideVM, [&]() {
            env->internal->elysium->oopManager->oopAccessPointerField(handleFetch(obj), fieldID->offset,
                                                                      handleFetch(val));
        });
    };
    env.internal->SetBooleanField = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID,
                                       jboolean val) {
        auto ptr = env->internal->elysium->oopManager->oopAccessField(handleFetch(obj), fieldID->offset);
        *reinterpret_cast<jboolean *>(ptr) = val;
    };
    env.internal->SetIntField = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jint val) {
        auto ptr = env->internal->elysium->oopManager->oopAccessField(handleFetch(obj), fieldID->offset);
        *reinterpret_cast<jint *>(ptr) = val;
    };
    env.internal->SetLongField = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jlong val) {
        auto ptr = env->internal->elysium->oopManager->oopAccessField(handleFetch(obj), fieldID->offset);
        *reinterpret_cast<jlong *>(ptr) = val;
    };

    env.internal->SetStaticObjectField = [](OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID,
                                            OMElysiaNativeHandle *value) {
        execWithState(InsideVM, [&]() {
            env->internal->elysium->oopManager->oopAccessPointerStaticField(clazz, fieldID->offset, handleFetch(value));
        });
    };

    env.internal->NewStringUTF = [](OMElysiaJNIEnv *env, const char *string) {
        std::string ss(string);
        OMElysiaNativeHandle *hnd;
        execWithState(InsideVM, [&]() {
            hnd = env->internal->elysium->executor->recordLocalRef(
                env->internal->elysium->oopManager->allocateString(ss));
        });
        return hnd;
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

        char *result;
        execWithState(InsideVM, [&]() {
            auto len = env->internal->elysium->oopManager->arrLength(handleFetch(arrdata));
            auto s = encoding::utf16ToUtf8New(data, len);
            result = reinterpret_cast<char *>(mem::allocator::tracedMallocElysia(s.size() + 1));
            std::memcpy(result, s.c_str(), s.size());
            result[s.size()] = '\0';
        });
        return reinterpret_cast<const char *>(result);
    };
    env.internal->ReleaseStringUTFChars = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *str, const char *chars) {
        mem::allocator::tracedFreeElysia(const_cast<char *>(chars));
    };

    env.internal->GetArrayLength = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *hnd) {
        return env->internal->elysium->oopManager->arrLength(handleFetch(hnd));
    };

    env.internal->NewObjectArray = [](OMElysiaJNIEnv *env, jsize len, OMElysiaKlass *klass,
                                      OMElysiaNativeHandle *init) {
        OMElysiaNativeHandle *hnd;
        execWithState(InsideVM, [&]() {
            hnd = env->internal->elysium->executor->recordLocalRef(
                env->internal->elysium->oopManager->allocateArr(env->internal->elysium->executor->currentKlassloader()
                                                                    ->fetchOrLoadClass(buildArray(klass->name))
                                                                    ->toArray(),
                                                                len));
            for (int i = 0; i < len; i++)
            {
                env->internal->elysium->oopManager->arrAccessPtr(hnd->object, i, handleFetch(init));
            }
        });
        return hnd;
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
        OMElysiaNativeHandle *hnd;
        execWithState(InsideVM, [&]() {
            hnd = env->internal->elysium->executor->recordLocalRef(env->internal->elysium->oopManager->allocateArr(
                env->internal->elysium->klassLoader->findClass("[C")->toArray(), len));
        });
        return hnd;
    };

    env.internal->GetByteArrayElements = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *isCopy) {
        jbyte *result;
        execWithState(InsideVM, [&]() {
            result = env->internal->elysium->oopManager->arrAccess<jbyte>(handleFetch(array));
            if (isCopy)
            {
                *isCopy = false;
            }
            atomic::atomic_store(&handleFetch(array)->markword,
                                 atomic::atomic_load(&handleFetch(array)->markword) | markFixed);
        });
        return result;
    };

    // TODO: copy impl!
    env.internal->GetCharArrayElements = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *isCopy) {
        jchar *result;
        execWithState(InsideVM, [&]() {
            result = env->internal->elysium->oopManager->arrAccess<jchar>(handleFetch(array));
            if (isCopy)
            {
                *isCopy = false;
            }
            atomic::atomic_store(&handleFetch(array)->markword,
                                 atomic::atomic_load(&handleFetch(array)->markword) | markFixed);
        });
        return result;
    };
    env.internal->ReleaseByteArrayElements = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *arr, jbyte *arrn, jint i) {
        execWithState(InsideVM, [&]() {
            atomic::atomic_store(&handleFetch(arr)->markword,
                                 atomic::atomic_load(&handleFetch(arr)->markword) & ~markFixed);
        });
    };

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
        });
        return 0;
    };
    env.internal->UnregisterNatives = [](OMElysiaJNIEnv *env, OMElysiaKlass *clazz) {
        execWithState(InsideVM, [&]() {
            clazz->nativeMethodCount = 0;
            mem::allocator::tracedFreeElysia(clazz->nativeMethods);
            clazz->nativeMethods = nullptr;
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
