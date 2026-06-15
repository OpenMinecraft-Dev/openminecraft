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
static OMElysiaNativeHandle *interfaceCallObjectMethodA(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj,
                                                        OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
{
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

void initBaseInterface(OMElysiaJNIEnv env)
{
    env.internal->GetVersion = [](OMElysiaJNIEnv *) { return JNI_VERSION_1_8; };

    // TODO: use current klass loader
    env.internal->FindClass = [](OMElysiaJNIEnv *env, const char *name) {
        OMElysiaKlass *klass;
        execWithState(InsideVM,
                      [&]() { klass = env->internal->elysium->klassLoader->fetchOrLoadClass(std::string(name)); });
        return klass;
    };
    env.internal->GetSuperclass = [](OMElysiaJNIEnv *env, OMElysiaKlass *klass) { return klass->superClass; };
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
    env.internal->CallObjectMethodA = interfaceCallObjectMethodA;

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
    env.internal->SetIntField = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jint val) {
        auto ptr = env->internal->elysium->oopManager->oopAccessField(handleFetch(obj), fieldID->offset);
        *reinterpret_cast<jint *>(ptr) = val;
    };
    env.internal->SetLongField = [](OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jlong val) {
        auto ptr = env->internal->elysium->oopManager->oopAccessField(handleFetch(obj), fieldID->offset);
        *reinterpret_cast<jlong *>(ptr) = val;
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

    env.internal->NewCharArray = [](OMElysiaJNIEnv *env, jsize len) {
        OMElysiaNativeHandle *hnd;
        execWithState(InsideVM, [&]() {
            hnd = env->internal->elysium->executor->recordLocalRef(env->internal->elysium->oopManager->allocateArr(
                env->internal->elysium->klassLoader->findClass("[C")->toArray(), len));
        });
        return hnd;
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
}
} // namespace openminecraft::vm::elysia
