#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/om_elysia_field.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <cstring>

namespace openminecraft::vm::elysia
{
void initBaseInterface(OMElysiaJNIEnv env)
{
    env->GetVersion = [](OMElysiaJNIEnv *) { return JNI_VERSION_1_8; };
    env->RegisterNatives = [](OMElysiaJNIEnv *env, OMElysiaKlass *clazz, const OMElysiaNativeMethod *methods,
                              jint nMethods) {
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
        return 0;
    };
    env->FindClass = [](OMElysiaJNIEnv *env, const char *name) {
        beg:
            auto klass = (*env)->world->klassLoader->findClass(std::string(name));
            if (!klass)
            {
                (*env)->world->klassLoader->loadClass(std::string(name));
                goto beg;
            }
            else
            {
                return klass;
            }
    };
    env->GetSuperclass = [](OMElysiaJNIEnv *env, OMElysiaKlass *klass) { return klass->superClass; };
    env->GetFieldID = [](OMElysiaJNIEnv *env, OMElysiaKlass *clazz, const char *name,
                         const char *desc) -> OMElysiaField * { return clazz->toInstance()->findField(name, desc); };
}
} // namespace openminecraft::vm::elysia
