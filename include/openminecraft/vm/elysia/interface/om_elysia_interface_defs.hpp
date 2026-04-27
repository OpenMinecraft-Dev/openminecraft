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

struct OMElysiaNativeHandle
{
    OMElysiaOop *object;
};

struct OMElysiaNativeMethod
{
    char *name;
    char *signature;
    void *funcPtr;
};

union OMElysiaNativeValue
{
    jboolean z;
    jbyte b;
    jshort s;
    jchar c;
    jint i;
    jfloat f;
    jlong j;
    jdouble d;
    OMElysiaNativeHandle *l;
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
    OMElysiaKlass *(*DefineClass)(OMElysiaJNIEnv *env, const char *name, OMElysiaNativeHandle *loader, const jbyte *buf);
    OMElysiaKlass *(*FindClass)(OMElysiaJNIEnv *env, const char *name);

    OMElysiaMethod *(*FromReflectedMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *method);
    OMElysiaField *(*FromReflectedField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *field);

    OMElysiaOop *(*ToReflectedMethod)(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaMethod *method,
                                      jboolean isStatic);

    OMElysiaKlass *(*GetSuperclass)(OMElysiaJNIEnv *env, OMElysiaKlass *klass);
    jboolean (*IsAssignableFrom)(OMElysiaJNIEnv *env, OMElysiaKlass *sub, OMElysiaKlass *sup);

    OMElysiaNativeHandle *(*ToReflectedField)(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaField *field, jboolean isStatic);
};
} // namespace openminecraft::vm::elysia

#endif
