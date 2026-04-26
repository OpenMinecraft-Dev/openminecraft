#ifndef OM_ELYSIA_INTERFACE_DEFS_HPP
#define OM_ELYSIA_INTERFACE_DEFS_HPP

#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include <ffi.h>

namespace openminecraft::vm::elysia
{
typedef jint jsize;
class OMElysiaKlass;
class OMElysiaOop;
class OMElysiaMethod;
class OMElysiaField;

struct OMElysiaNativeMethod
{
    char *name;
    char *signature;
    void *funcPtr;
};

struct OMElysiaNativeInterface;
typedef OMElysiaNativeInterface *OMElysiaJNIEnv;

struct OMElysiaNativeInterface
{
    void *reserve0;
    void *reserve1;
    void *reserve2;
    void *reserve3;

    jint (*GetVersion)(OMElysiaJNIEnv *env);
    OMElysiaKlass *(*DefineClass)(OMElysiaJNIEnv *env, const char *name, OMElysiaOop *loader, const jbyte *buf);
    OMElysiaKlass *(*FindClass)(OMElysiaJNIEnv *env, const char *name);

    OMElysiaMethod *(*FromReflectedMethod)(OMElysiaJNIEnv *env, OMElysiaOop *method);
    OMElysiaField *(*FromReflectedField)(OMElysiaJNIEnv *env, OMElysiaOop *field);

    OMElysiaOop *(*ToReflectedMethod)(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaMethod *method,
                                      jboolean isStatic);

    OMElysiaKlass *(*GetSuperclass)(OMElysiaJNIEnv *env, OMElysiaKlass *klass);
    jboolean (*IsAssignableFrom)(OMElysiaJNIEnv *env, OMElysiaKlass *sub, OMElysiaKlass *sup);
};
} // namespace openminecraft::vm::elysia

#endif
