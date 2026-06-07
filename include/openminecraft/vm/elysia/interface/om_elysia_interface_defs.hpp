#ifndef OM_ELYSIA_INTERFACE_DEFS_HPP
#define OM_ELYSIA_INTERFACE_DEFS_HPP

#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include <stdarg.h>

namespace openminecraft::vm::elysia
{
#define JNI_VERSION_1_1 0x00010001
#define JNI_VERSION_1_2 0x00010002
#define JNI_VERSION_1_4 0x00010004
#define JNI_VERSION_1_6 0x00010006
#define JNI_VERSION_1_8 0x00010008
#define JNI_VERSION_9 0x00090000
#define JNI_VERSION_10 0x000a0000
#define JNI_VERSION_19 0x00130000
#define JNI_VERSION_20 0x00140000
#define JNI_VERSION_21 0x00150000

typedef jint jsize;
class OMElysiaKlass;
class OMElysiaOop;
class OMElysiaMethod;
class OMElysiaField;
class OMElysium;

struct OMElysiaNativeHandle
{
    OMElysiaOop *object;
    OMElysiaNativeHandle *next;
};

struct OMElysiaNativeMethod
{
    char *name;
    char *signature;
    void *funcPtr;
};

union OMElysiaNativeValue {
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
#ifndef __cplusplus
typedef OMElysiaNativeInterface *OMElysiaJNIEnv;
#else
struct OMElysiaJNIEnv;
#endif

void initBaseInterface(OMElysiaJNIEnv env);

struct OMElysiaNativeInterface
{
    OMElysium *elysium;
    void *reserve1;
    void *reserve2;
    void *reserve3;

    jint (*GetVersion)(OMElysiaJNIEnv *env);
    OMElysiaKlass *(*DefineClass)(OMElysiaJNIEnv *env, const char *name, OMElysiaNativeHandle *loader,
                                  const jbyte *buf);
    OMElysiaKlass *(*FindClass)(OMElysiaJNIEnv *env, const char *name);

    OMElysiaMethod *(*FromReflectedMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *method);
    OMElysiaField *(*FromReflectedField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *field);

    OMElysiaOop *(*ToReflectedMethod)(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaMethod *method,
                                      jboolean isStatic);

    OMElysiaKlass *(*GetSuperclass)(OMElysiaJNIEnv *env, OMElysiaKlass *klass);
    jboolean (*IsAssignableFrom)(OMElysiaJNIEnv *env, OMElysiaKlass *sub, OMElysiaKlass *sup);

    OMElysiaNativeHandle *(*ToReflectedField)(OMElysiaJNIEnv *env, OMElysiaKlass *klass, OMElysiaField *field,
                                              jboolean isStatic);

    jint (*Throw)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj);
    jint (*ThrowNew)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, const char *msg);
    OMElysiaNativeHandle *(*ExceptionOccurred)(OMElysiaJNIEnv *env);
    void (*ExceptionDescribe)(OMElysiaJNIEnv *env);
    void (*ExceptionClear)(OMElysiaJNIEnv *env);
    void (*FatalError)(OMElysiaJNIEnv *env, const char *msg);

    jint (*PushLocalFrame)(OMElysiaJNIEnv *env, jint capacity);
    OMElysiaNativeHandle *(*PopLocalFrame)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *result);

    OMElysiaNativeHandle *(*NewGlobalRef)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *lobj);
    void (*DeleteGlobalRef)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *gref);
    void (*DeleteLocalRef)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj);
    jboolean (*IsSameObject)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj1, OMElysiaNativeHandle *obj2);
    OMElysiaNativeHandle *(*NewLocalRef)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *ref);
    jint (*EnsureLocalCapacity)(OMElysiaJNIEnv *env, jint capacity);

    OMElysiaNativeHandle *(*AllocObject)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz);
    OMElysiaNativeHandle *(*NewObject)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, ...);
    OMElysiaNativeHandle *(*NewObjectV)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                        va_list args);
    OMElysiaNativeHandle *(*NewObjectA)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                        const OMElysiaNativeValue *args);

    OMElysiaKlass *(*GetObjectClass)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj);
    jboolean (*IsInstanceOf)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz);

    OMElysiaMethod *(*GetMethodID)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, const char *name, const char *sig);

    OMElysiaNativeHandle *(*CallObjectMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                                              ...);
    OMElysiaNativeHandle *(*CallObjectMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                                               va_list args);
    OMElysiaNativeHandle *(*CallObjectMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                                               const OMElysiaNativeValue *args);

    jboolean (*CallBooleanMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, ...);
    jboolean (*CallBooleanMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                                   va_list args);
    jboolean (*CallBooleanMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                                   const OMElysiaNativeValue *args);

    jbyte (*CallByteMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, ...);
    jbyte (*CallByteMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args);
    jbyte (*CallByteMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                             const OMElysiaNativeValue *args);

    jchar (*CallCharMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, ...);
    jchar (*CallCharMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args);
    jchar (*CallCharMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                             const OMElysiaNativeValue *args);

    jshort (*CallShortMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, ...);
    jshort (*CallShortMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args);
    jshort (*CallShortMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                               const OMElysiaNativeValue *args);

    jint (*CallIntMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, ...);
    jint (*CallIntMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args);
    jint (*CallIntMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                           const OMElysiaNativeValue *args);

    jlong (*CallLongMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, ...);
    jlong (*CallLongMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args);
    jlong (*CallLongMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                             const OMElysiaNativeValue *args);

    jfloat (*CallFloatMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, ...);
    jfloat (*CallFloatMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args);
    jfloat (*CallFloatMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                               const OMElysiaNativeValue *args);

    jdouble (*CallDoubleMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, ...);
    jdouble (*CallDoubleMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                                 va_list args);
    jdouble (*CallDoubleMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                                 const OMElysiaNativeValue *args);

    void (*CallVoidMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, ...);
    void (*CallVoidMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args);
    void (*CallVoidMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                            const OMElysiaNativeValue *args);

    OMElysiaNativeHandle *(*CallNonvirtualObjectMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj,
                                                        OMElysiaKlass *clazz, OMElysiaMethod *methodID, ...);
    OMElysiaNativeHandle *(*CallNonvirtualObjectMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj,
                                                         OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args);
    OMElysiaNativeHandle *(*CallNonvirtualObjectMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj,
                                                         OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                                         const OMElysiaNativeValue *args);

    jboolean (*CallNonvirtualBooleanMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                            OMElysiaMethod *methodID, ...);
    jboolean (*CallNonvirtualBooleanMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                             OMElysiaMethod *methodID, va_list args);
    jboolean (*CallNonvirtualBooleanMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                             OMElysiaMethod *methodID, const OMElysiaNativeValue *args);

    jbyte (*CallNonvirtualByteMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                      OMElysiaMethod *methodID, ...);
    jbyte (*CallNonvirtualByteMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                       OMElysiaMethod *methodID, va_list args);
    jbyte (*CallNonvirtualByteMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                       OMElysiaMethod *methodID, const OMElysiaNativeValue *args);

    jchar (*CallNonvirtualCharMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                      OMElysiaMethod *methodID, ...);
    jchar (*CallNonvirtualCharMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                       OMElysiaMethod *methodID, va_list args);
    jchar (*CallNonvirtualCharMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                       OMElysiaMethod *methodID, const OMElysiaNativeValue *args);

    jshort (*CallNonvirtualShortMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                        OMElysiaMethod *methodID, ...);
    jshort (*CallNonvirtualShortMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                         OMElysiaMethod *methodID, va_list args);
    jshort (*CallNonvirtualShortMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                         OMElysiaMethod *methodID, const OMElysiaNativeValue *args);

    jint (*CallNonvirtualIntMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                    OMElysiaMethod *methodID, ...);
    jint (*CallNonvirtualIntMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                     OMElysiaMethod *methodID, va_list args);
    jint (*CallNonvirtualIntMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                     OMElysiaMethod *methodID, const OMElysiaNativeValue *args);

    jlong (*CallNonvirtualLongMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                      OMElysiaMethod *methodID, ...);
    jlong (*CallNonvirtualLongMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                       OMElysiaMethod *methodID, va_list args);
    jlong (*CallNonvirtualLongMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                       OMElysiaMethod *methodID, const OMElysiaNativeValue *args);

    jfloat (*CallNonvirtualFloatMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                        OMElysiaMethod *methodID, ...);
    jfloat (*CallNonvirtualFloatMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                         OMElysiaMethod *methodID, va_list args);
    jfloat (*CallNonvirtualFloatMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                         OMElysiaMethod *methodID, const OMElysiaNativeValue *args);

    jdouble (*CallNonvirtualDoubleMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                          OMElysiaMethod *methodID, ...);
    jdouble (*CallNonvirtualDoubleMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                           OMElysiaMethod *methodID, va_list args);
    jdouble (*CallNonvirtualDoubleMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                           OMElysiaMethod *methodID, const OMElysiaNativeValue *args);

    void (*CallNonvirtualVoidMethod)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                     OMElysiaMethod *methodID, ...);
    void (*CallNonvirtualVoidMethodV)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                      OMElysiaMethod *methodID, va_list args);
    void (*CallNonvirtualVoidMethodA)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                      OMElysiaMethod *methodID, const OMElysiaNativeValue *args);

    OMElysiaField *(*GetFieldID)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, const char *name, const char *sig);

    OMElysiaNativeHandle *(*GetObjectField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID);
    jboolean (*GetBooleanField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID);
    jbyte (*GetByteField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID);
    jchar (*GetCharField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID);
    jshort (*GetShortField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID);
    jint (*GetIntField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID);
    jlong (*GetLongField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID);
    jfloat (*GetFloatField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID);
    jdouble (*GetDoubleField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID);

    void (*SetObjectField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID,
                           OMElysiaNativeHandle *val);
    void (*SetBooleanField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jboolean val);
    void (*SetByteField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jbyte val);
    void (*SetCharField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jchar val);
    void (*SetShortField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jshort val);
    void (*SetIntField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jint val);
    void (*SetLongField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jlong val);
    void (*SetFloatField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jfloat val);
    void (*SetDoubleField)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jdouble val);

    OMElysiaMethod *(*GetStaticMethodID)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, const char *name, const char *sig);

    OMElysiaNativeHandle *(*CallStaticObjectMethod)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                                    ...);
    OMElysiaNativeHandle *(*CallStaticObjectMethodV)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz,
                                                     OMElysiaMethod *methodID, va_list args);
    OMElysiaNativeHandle *(*CallStaticObjectMethodA)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz,
                                                     OMElysiaMethod *methodID, const OMElysiaNativeValue *args);

    jboolean (*CallStaticBooleanMethod)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, ...);
    jboolean (*CallStaticBooleanMethodV)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                         va_list args);
    jboolean (*CallStaticBooleanMethodA)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                         const OMElysiaNativeValue *args);

    jbyte (*CallStaticByteMethod)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, ...);
    jbyte (*CallStaticByteMethodV)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args);
    jbyte (*CallStaticByteMethodA)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                   const OMElysiaNativeValue *args);

    jchar (*CallStaticCharMethod)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, ...);
    jchar (*CallStaticCharMethodV)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args);
    jchar (*CallStaticCharMethodA)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                   const OMElysiaNativeValue *args);

    jshort (*CallStaticShortMethod)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, ...);
    jshort (*CallStaticShortMethodV)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args);
    jshort (*CallStaticShortMethodA)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                     const OMElysiaNativeValue *args);

    jint (*CallStaticIntMethod)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, ...);
    jint (*CallStaticIntMethodV)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args);
    jint (*CallStaticIntMethodA)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                 const OMElysiaNativeValue *args);

    jlong (*CallStaticLongMethod)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, ...);
    jlong (*CallStaticLongMethodV)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args);
    jlong (*CallStaticLongMethodA)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                   const OMElysiaNativeValue *args);

    jfloat (*CallStaticFloatMethod)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, ...);
    jfloat (*CallStaticFloatMethodV)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args);
    jfloat (*CallStaticFloatMethodA)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                     const OMElysiaNativeValue *args);

    jdouble (*CallStaticDoubleMethod)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID, ...);
    jdouble (*CallStaticDoubleMethodV)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                       va_list args);
    jdouble (*CallStaticDoubleMethodA)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                       const OMElysiaNativeValue *args);

    void (*CallStaticVoidMethod)(OMElysiaJNIEnv *env, OMElysiaKlass *cls, OMElysiaMethod *methodID, ...);
    void (*CallStaticVoidMethodV)(OMElysiaJNIEnv *env, OMElysiaKlass *cls, OMElysiaMethod *methodID, va_list args);
    void (*CallStaticVoidMethodA)(OMElysiaJNIEnv *env, OMElysiaKlass *cls, OMElysiaMethod *methodID,
                                  const OMElysiaNativeValue *args);

    OMElysiaField *(*GetStaticFieldID)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, const char *name, const char *sig);
    OMElysiaNativeHandle *(*GetStaticObjectField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID);
    jboolean (*GetStaticBooleanField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID);
    jbyte (*GetStaticByteField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID);
    jchar (*GetStaticCharField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID);
    jshort (*GetStaticShortField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID);
    jint (*GetStaticIntField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID);
    jlong (*GetStaticLongField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID);
    jfloat (*GetStaticFloatField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID);
    jdouble (*GetStaticDoubleField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID);

    void (*SetStaticObjectField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID,
                                 OMElysiaNativeHandle *value);
    void (*SetStaticBooleanField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID, jboolean value);
    void (*SetStaticByteField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID, jbyte value);
    void (*SetStaticCharField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID, jchar value);
    void (*SetStaticShortField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID, jshort value);
    void (*SetStaticIntField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID, jint value);
    void (*SetStaticLongField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID, jlong value);
    void (*SetStaticFloatField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID, jfloat value);
    void (*SetStaticDoubleField)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, OMElysiaField *fieldID, jdouble value);

    OMElysiaNativeHandle *(*NewString)(OMElysiaJNIEnv *env, const jchar *unicode, jsize len);
    jsize (*GetStringLength)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *str);
    const jchar *(*GetStringChars)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *str, jboolean *isCopy);
    void (*ReleaseStringChars)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *str, const jchar *chars);

    OMElysiaNativeHandle *(*NewStringUTF)(OMElysiaJNIEnv *env, const char *utf);
    jsize (*GetStringUTFLength)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *str);
    const char *(*GetStringUTFChars)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *str, jboolean *isCopy);
    void (*ReleaseStringUTFChars)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *str, const char *chars);

    jsize (*GetArrayLength)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array);

    OMElysiaNativeHandle *(*NewObjectArray)(OMElysiaJNIEnv *env, jsize len, OMElysiaKlass *clazz,
                                            OMElysiaNativeHandle *init);
    OMElysiaNativeHandle *(*GetObjectArrayElement)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize index);
    void (*SetObjectArrayElement)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize index,
                                  OMElysiaNativeHandle *val);

    OMElysiaNativeHandle *(*NewBooleanArray)(OMElysiaJNIEnv *env, jsize len);
    OMElysiaNativeHandle *(*NewByteArray)(OMElysiaJNIEnv *env, jsize len);
    OMElysiaNativeHandle *(*NewCharArray)(OMElysiaJNIEnv *env, jsize len);
    OMElysiaNativeHandle *(*NewShortArray)(OMElysiaJNIEnv *env, jsize len);
    OMElysiaNativeHandle *(*NewIntArray)(OMElysiaJNIEnv *env, jsize len);
    OMElysiaNativeHandle *(*NewLongArray)(OMElysiaJNIEnv *env, jsize len);
    OMElysiaNativeHandle *(*NewFloatArray)(OMElysiaJNIEnv *env, jsize len);
    OMElysiaNativeHandle *(*NewDoubleArray)(OMElysiaJNIEnv *env, jsize len);

    jboolean *(*GetBooleanArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *isCopy);
    jbyte *(*GetByteArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *isCopy);
    jchar *(*GetCharArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *isCopy);
    jshort *(*GetShortArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *isCopy);
    jint *(*GetIntArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *isCopy);
    jlong *(*GetLongArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *isCopy);
    jfloat *(*GetFloatArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *isCopy);
    jdouble *(*GetDoubleArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *isCopy);

    void (*ReleaseBooleanArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *elems, jint mode);
    void (*ReleaseByteArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jbyte *elems, jint mode);
    void (*ReleaseCharArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jchar *elems, jint mode);
    void (*ReleaseShortArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jshort *elems, jint mode);
    void (*ReleaseIntArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jint *elems, jint mode);
    void (*ReleaseLongArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jlong *elems, jint mode);
    void (*ReleaseFloatArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jfloat *elems, jint mode);
    void (*ReleaseDoubleArrayElements)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jdouble *elems, jint mode);

    void (*GetBooleanArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize l,
                                  jboolean *buf);
    void (*GetByteArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize len, jbyte *buf);
    void (*GetCharArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize len, jchar *buf);
    void (*GetShortArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize len, jshort *buf);
    void (*GetIntArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize len, jint *buf);
    void (*GetLongArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize len, jlong *buf);
    void (*GetFloatArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize len, jfloat *buf);
    void (*GetDoubleArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize len,
                                 jdouble *buf);

    void (*SetBooleanArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize l,
                                  const jboolean *buf);
    void (*SetByteArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize len,
                               const jbyte *buf);
    void (*SetCharArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize len,
                               const jchar *buf);
    void (*SetShortArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize len,
                                const jshort *buf);
    void (*SetIntArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize len,
                              const jint *buf);
    void (*SetLongArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize len,
                               const jlong *buf);
    void (*SetFloatArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize len,
                                const jfloat *buf);
    void (*SetDoubleArrayRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jsize start, jsize len,
                                 const jdouble *buf);

    jint (*RegisterNatives)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz, const OMElysiaNativeMethod *methods,
                            jint nMethods);
    jint (*UnregisterNatives)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz);

    jint (*MonitorEnter)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj);
    jint (*MonitorExit)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj);

    // TODO: VM struct def
    jint (*GetJavaVM)(OMElysiaJNIEnv *env, void **vm);

    void (*GetStringRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *str, jsize start, jsize len, jchar *buf);
    void (*GetStringUTFRegion)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *str, jsize start, jsize len, char *buf);

    void *(*GetPrimitiveArrayCritical)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, jboolean *isCopy);
    void (*ReleasePrimitiveArrayCritical)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *array, void *carray, jint mode);

    const jchar *(*GetStringCritical)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *string, jboolean *isCopy);
    void (*ReleaseStringCritical)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *string, const jchar *cstring);

    OMElysiaNativeHandle *(*NewWeakGlobalRef)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj);
    void (*DeleteWeakGlobalRef)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *ref);

    jboolean (*ExceptionCheck)(OMElysiaJNIEnv *env);

    OMElysiaNativeHandle *(*NewDirectByteBuffer)(OMElysiaJNIEnv *env, void *address, jlong capacity);
    void *(*GetDirectBufferAddress)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *buf);
    jlong (*GetDirectBufferCapacity)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *buf);

    /* New JNI 1.6 Features */

    // TODO: Ref type
    jint (*GetObjectRefType)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj);

    /* Module Features */

    OMElysiaNativeHandle *(*GetModule)(OMElysiaJNIEnv *env, OMElysiaKlass *clazz);

    /* Virtual threads */

    jboolean (*IsVirtualThread)(OMElysiaJNIEnv *env, OMElysiaNativeHandle *obj);
};

#ifdef __cplusplus
struct OMElysiaJNIEnv
{
    OMElysiaNativeInterface *internal;
    OMElysiaKlass *FindClass(const char *name)
    {
        return internal->FindClass(this, name);
    }

    jint RegisterNatives(OMElysiaKlass *clazz, const OMElysiaNativeMethod *methods, jint nMethods)
    {
        return internal->RegisterNatives(this, clazz, methods, nMethods);
    }

    OMElysiaField *GetFieldID(OMElysiaKlass *clazz, const char *name, const char *sig)
    {
        return internal->GetFieldID(this, clazz, name, sig);
    }

    OMElysiaNativeHandle *AllocObject(OMElysiaKlass *clazz)
    {
        return internal->AllocObject(this, clazz);
    }

    jint GetVersion()
    {
        return internal->GetVersion(this);
    }

    jint UnregisterNatives(OMElysiaKlass *clazz)
    {
        return internal->UnregisterNatives(this, clazz);
    }

    OMElysiaKlass *GetSuperclass(OMElysiaKlass *clazz)
    {
        return internal->GetSuperclass(this, clazz);
    }

    OMElysiaNativeHandle *NewCharArray(jsize len)
    {
        return internal->NewCharArray(this, len);
    }

    OMElysiaNativeHandle *NewStringUTF(const char *string)
    {
        return internal->NewStringUTF(this, string);
    }

    OMElysiaNativeHandle *GetObjectField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID)
    {
        return internal->GetObjectField(this, obj, fieldID);
    }

    jchar *GetCharArrayElements(OMElysiaNativeHandle *array, jboolean *isCopy)
    {
        return internal->GetCharArrayElements(this, array, isCopy);
    }

    const char *GetStringUTFChars(OMElysiaNativeHandle *str, jboolean *isCopy)
    {
        return internal->GetStringUTFChars(this, str, isCopy);
    }

    void ReleaseStringUTFChars(OMElysiaNativeHandle *str, const char *chars)
    {
        internal->ReleaseStringUTFChars(this, str, chars);
    }

    void SetObjectField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID, OMElysiaNativeHandle *val)
    {
        internal->SetObjectField(this, obj, fieldID, val);
    }

    OMElysiaNativeHandle *CallObjectMethodA(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallObjectMethodA(this, obj, methodID, args);
    }

    OMElysiaMethod *GetMethodID(OMElysiaKlass *clazz, const char *name, const char *sig)
    {
        return internal->GetMethodID(this, clazz, name, sig);
    }

    jboolean IsVirtualThread(OMElysiaNativeHandle *obj)
{
    return internal->IsVirtualThread(this, obj);
}
};
#endif
} // namespace openminecraft::vm::elysia

#endif
