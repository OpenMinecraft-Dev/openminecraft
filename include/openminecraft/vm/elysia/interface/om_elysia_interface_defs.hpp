#ifndef OM_ELYSIA_INTERFACE_DEFS_HPP
#define OM_ELYSIA_INTERFACE_DEFS_HPP

#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include <stdarg.h>
#include <utility>

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

#define handleFetch(h) (h ? h->object : nullptr)

static OMElysiaNativeHandle *createTempHandle(OMElysiaOop *oop)
{
    auto hnd = reinterpret_cast<OMElysiaNativeHandle *>(malloc(sizeof(OMElysiaNativeHandle)));
    hnd->next = hnd;
    hnd->object = oop;
    return hnd;
}

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
    jint GetVersion()
    {
        return internal->GetVersion(this);
    }
    OMElysiaKlass *DefineClass(const char *name, OMElysiaNativeHandle *loader, const jbyte *buf)
    {
        return internal->DefineClass(this, name, loader, buf);
    }
    OMElysiaKlass *FindClass(const char *name)
    {
        return internal->FindClass(this, name);
    }
    OMElysiaMethod *FromReflectedMethod(OMElysiaNativeHandle *method)
    {
        return internal->FromReflectedMethod(this, method);
    }
    OMElysiaField *FromReflectedField(OMElysiaNativeHandle *field)
    {
        return internal->FromReflectedField(this, field);
    }
    OMElysiaOop *ToReflectedMethod(OMElysiaKlass *klass, OMElysiaMethod *method, jboolean isStatic)
    {
        return internal->ToReflectedMethod(this, klass, method, isStatic);
    }
    OMElysiaKlass *GetSuperclass(OMElysiaKlass *klass)
    {
        return internal->GetSuperclass(this, klass);
    }
    jboolean IsAssignableFrom(OMElysiaKlass *sub, OMElysiaKlass *sup)
    {
        return internal->IsAssignableFrom(this, sub, sup);
    }
    OMElysiaNativeHandle *ToReflectedField(OMElysiaKlass *klass, OMElysiaField *field, jboolean isStatic)
    {
        return internal->ToReflectedField(this, klass, field, isStatic);
    }
    jint Throw(OMElysiaNativeHandle *obj)
    {
        return internal->Throw(this, obj);
    }
    jint ThrowNew(OMElysiaKlass *clazz, const char *msg)
    {
        return internal->ThrowNew(this, clazz, msg);
    }
    OMElysiaNativeHandle *ExceptionOccurred()
    {
        return internal->ExceptionOccurred(this);
    }
    void ExceptionDescribe()
    {
        internal->ExceptionDescribe(this);
    }
    void ExceptionClear()
    {
        internal->ExceptionClear(this);
    }
    void FatalError(const char *msg)
    {
        internal->FatalError(this, msg);
    }
    jint PushLocalFrame(jint capacity)
    {
        return internal->PushLocalFrame(this, capacity);
    }
    OMElysiaNativeHandle *PopLocalFrame(OMElysiaNativeHandle *result)
    {
        return internal->PopLocalFrame(this, result);
    }
    OMElysiaNativeHandle *NewGlobalRef(OMElysiaNativeHandle *lobj)
    {
        return internal->NewGlobalRef(this, lobj);
    }
    void DeleteGlobalRef(OMElysiaNativeHandle *gref)
    {
        internal->DeleteGlobalRef(this, gref);
    }
    void DeleteLocalRef(OMElysiaNativeHandle *obj)
    {
        internal->DeleteLocalRef(this, obj);
    }
    jboolean IsSameObject(OMElysiaNativeHandle *obj1, OMElysiaNativeHandle *obj2)
    {
        return internal->IsSameObject(this, obj1, obj2);
    }
    OMElysiaNativeHandle *NewLocalRef(OMElysiaNativeHandle *ref)
    {
        return internal->NewLocalRef(this, ref);
    }
    jint EnsureLocalCapacity(jint capacity)
    {
        return internal->EnsureLocalCapacity(this, capacity);
    }
    OMElysiaNativeHandle *AllocObject(OMElysiaKlass *clazz)
    {
        return internal->AllocObject(this, clazz);
    }
    template <typename... Ts>
    OMElysiaNativeHandle *NewObject(OMElysiaKlass *clazz, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->NewObject(this, clazz, methodID, std::forward<Ts>(args)...);
    }
    OMElysiaNativeHandle *NewObjectV(OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args)
    {
        return internal->NewObjectV(this, clazz, methodID, args);
    }
    OMElysiaNativeHandle *NewObjectA(OMElysiaKlass *clazz, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->NewObjectA(this, clazz, methodID, args);
    }
    OMElysiaKlass *GetObjectClass(OMElysiaNativeHandle *obj)
    {
        return internal->GetObjectClass(this, obj);
    }
    jboolean IsInstanceOf(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz)
    {
        return internal->IsInstanceOf(this, obj, clazz);
    }
    OMElysiaMethod *GetMethodID(OMElysiaKlass *clazz, const char *name, const char *sig)
    {
        return internal->GetMethodID(this, clazz, name, sig);
    }
    template <typename... Ts>
    OMElysiaNativeHandle *CallObjectMethod(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallObjectMethod(this, obj, methodID, std::forward<Ts>(args)...);
    }
    OMElysiaNativeHandle *CallObjectMethodV(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallObjectMethodV(this, obj, methodID, args);
    }
    OMElysiaNativeHandle *CallObjectMethodA(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID,
                                            const OMElysiaNativeValue *args)
    {
        return internal->CallObjectMethodA(this, obj, methodID, args);
    }
    template <typename... Ts>
    jboolean CallBooleanMethod(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallBooleanMethod(this, obj, methodID, std::forward<Ts>(args)...);
    }
    jboolean CallBooleanMethodV(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallBooleanMethodV(this, obj, methodID, args);
    }
    jboolean CallBooleanMethodA(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallBooleanMethodA(this, obj, methodID, args);
    }
    template <typename... Ts> jbyte CallByteMethod(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallByteMethod(this, obj, methodID, std::forward<Ts>(args)...);
    }
    jbyte CallByteMethodV(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallByteMethodV(this, obj, methodID, args);
    }
    jbyte CallByteMethodA(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallByteMethodA(this, obj, methodID, args);
    }
    template <typename... Ts> jchar CallCharMethod(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallCharMethod(this, obj, methodID, std::forward<Ts>(args)...);
    }
    jchar CallCharMethodV(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallCharMethodV(this, obj, methodID, args);
    }
    jchar CallCharMethodA(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallCharMethodA(this, obj, methodID, args);
    }
    template <typename... Ts> jshort CallShortMethod(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallShortMethod(this, obj, methodID, std::forward<Ts>(args)...);
    }
    jshort CallShortMethodV(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallShortMethodV(this, obj, methodID, args);
    }
    jshort CallShortMethodA(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallShortMethodA(this, obj, methodID, args);
    }
    template <typename... Ts> jint CallIntMethod(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallIntMethod(this, obj, methodID, std::forward<Ts>(args)...);
    }
    jint CallIntMethodV(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallIntMethodV(this, obj, methodID, args);
    }
    jint CallIntMethodA(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallIntMethodA(this, obj, methodID, args);
    }
    template <typename... Ts> jlong CallLongMethod(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallLongMethod(this, obj, methodID, std::forward<Ts>(args)...);
    }
    jlong CallLongMethodV(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallLongMethodV(this, obj, methodID, args);
    }
    jlong CallLongMethodA(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallLongMethodA(this, obj, methodID, args);
    }
    template <typename... Ts> jfloat CallFloatMethod(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallFloatMethod(this, obj, methodID, std::forward<Ts>(args)...);
    }
    jfloat CallFloatMethodV(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallFloatMethodV(this, obj, methodID, args);
    }
    jfloat CallFloatMethodA(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallFloatMethodA(this, obj, methodID, args);
    }
    template <typename... Ts> jdouble CallDoubleMethod(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallDoubleMethod(this, obj, methodID, std::forward<Ts>(args)...);
    }
    jdouble CallDoubleMethodV(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallDoubleMethodV(this, obj, methodID, args);
    }
    jdouble CallDoubleMethodA(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallDoubleMethodA(this, obj, methodID, args);
    }
    template <typename... Ts> void CallVoidMethod(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, Ts... args)
    {
        internal->CallVoidMethod(this, obj, methodID, std::forward<Ts>(args)...);
    }
    void CallVoidMethodV(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, va_list args)
    {
        internal->CallVoidMethodV(this, obj, methodID, args);
    }
    void CallVoidMethodA(OMElysiaNativeHandle *obj, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        internal->CallVoidMethodA(this, obj, methodID, args);
    }
    template <typename... Ts>
    OMElysiaNativeHandle *CallNonvirtualObjectMethod(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                                     OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallNonvirtualObjectMethod(this, obj, clazz, methodID, std::forward<Ts>(args)...);
    }
    OMElysiaNativeHandle *CallNonvirtualObjectMethodV(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                                      OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallNonvirtualObjectMethodV(this, obj, clazz, methodID, args);
    }
    OMElysiaNativeHandle *CallNonvirtualObjectMethodA(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz,
                                                      OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallNonvirtualObjectMethodA(this, obj, clazz, methodID, args);
    }
    template <typename... Ts>
    jboolean CallNonvirtualBooleanMethod(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                         Ts... args)
    {
        return internal->CallNonvirtualBooleanMethod(this, obj, clazz, methodID, std::forward<Ts>(args)...);
    }
    jboolean CallNonvirtualBooleanMethodV(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                          va_list args)
    {
        return internal->CallNonvirtualBooleanMethodV(this, obj, clazz, methodID, args);
    }
    jboolean CallNonvirtualBooleanMethodA(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                          const OMElysiaNativeValue *args)
    {
        return internal->CallNonvirtualBooleanMethodA(this, obj, clazz, methodID, args);
    }
    template <typename... Ts>
    jbyte CallNonvirtualByteMethod(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                   Ts... args)
    {
        return internal->CallNonvirtualByteMethod(this, obj, clazz, methodID, std::forward<Ts>(args)...);
    }
    jbyte CallNonvirtualByteMethodV(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                    va_list args)
    {
        return internal->CallNonvirtualByteMethodV(this, obj, clazz, methodID, args);
    }
    jbyte CallNonvirtualByteMethodA(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                    const OMElysiaNativeValue *args)
    {
        return internal->CallNonvirtualByteMethodA(this, obj, clazz, methodID, args);
    }
    template <typename... Ts>
    jchar CallNonvirtualCharMethod(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                   Ts... args)
    {
        return internal->CallNonvirtualCharMethod(this, obj, clazz, methodID, std::forward<Ts>(args)...);
    }
    jchar CallNonvirtualCharMethodV(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                    va_list args)
    {
        return internal->CallNonvirtualCharMethodV(this, obj, clazz, methodID, args);
    }
    jchar CallNonvirtualCharMethodA(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                    const OMElysiaNativeValue *args)
    {
        return internal->CallNonvirtualCharMethodA(this, obj, clazz, methodID, args);
    }
    template <typename... Ts>
    jshort CallNonvirtualShortMethod(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                     Ts... args)
    {
        return internal->CallNonvirtualShortMethod(this, obj, clazz, methodID, std::forward<Ts>(args)...);
    }
    jshort CallNonvirtualShortMethodV(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                      va_list args)
    {
        return internal->CallNonvirtualShortMethodV(this, obj, clazz, methodID, args);
    }
    jshort CallNonvirtualShortMethodA(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                      const OMElysiaNativeValue *args)
    {
        return internal->CallNonvirtualShortMethodA(this, obj, clazz, methodID, args);
    }
    template <typename... Ts>
    jint CallNonvirtualIntMethod(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallNonvirtualIntMethod(this, obj, clazz, methodID, std::forward<Ts>(args)...);
    }
    jint CallNonvirtualIntMethodV(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                  va_list args)
    {
        return internal->CallNonvirtualIntMethodV(this, obj, clazz, methodID, args);
    }
    jint CallNonvirtualIntMethodA(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                  const OMElysiaNativeValue *args)
    {
        return internal->CallNonvirtualIntMethodA(this, obj, clazz, methodID, args);
    }
    template <typename... Ts>
    jlong CallNonvirtualLongMethod(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                   Ts... args)
    {
        return internal->CallNonvirtualLongMethod(this, obj, clazz, methodID, std::forward<Ts>(args)...);
    }
    jlong CallNonvirtualLongMethodV(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                    va_list args)
    {
        return internal->CallNonvirtualLongMethodV(this, obj, clazz, methodID, args);
    }
    jlong CallNonvirtualLongMethodA(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                    const OMElysiaNativeValue *args)
    {
        return internal->CallNonvirtualLongMethodA(this, obj, clazz, methodID, args);
    }
    template <typename... Ts>
    jfloat CallNonvirtualFloatMethod(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                     Ts... args)
    {
        return internal->CallNonvirtualFloatMethod(this, obj, clazz, methodID, std::forward<Ts>(args)...);
    }
    jfloat CallNonvirtualFloatMethodV(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                      va_list args)
    {
        return internal->CallNonvirtualFloatMethodV(this, obj, clazz, methodID, args);
    }
    jfloat CallNonvirtualFloatMethodA(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                      const OMElysiaNativeValue *args)
    {
        return internal->CallNonvirtualFloatMethodA(this, obj, clazz, methodID, args);
    }
    template <typename... Ts>
    jdouble CallNonvirtualDoubleMethod(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                       Ts... args)
    {
        return internal->CallNonvirtualDoubleMethod(this, obj, clazz, methodID, std::forward<Ts>(args)...);
    }
    jdouble CallNonvirtualDoubleMethodV(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                        va_list args)
    {
        return internal->CallNonvirtualDoubleMethodV(this, obj, clazz, methodID, args);
    }
    jdouble CallNonvirtualDoubleMethodA(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                        const OMElysiaNativeValue *args)
    {
        return internal->CallNonvirtualDoubleMethodA(this, obj, clazz, methodID, args);
    }
    template <typename... Ts>
    void CallNonvirtualVoidMethod(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID, Ts... args)
    {
        internal->CallNonvirtualVoidMethod(this, obj, clazz, methodID, std::forward<Ts>(args)...);
    }
    void CallNonvirtualVoidMethodV(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                   va_list args)
    {
        internal->CallNonvirtualVoidMethodV(this, obj, clazz, methodID, args);
    }
    void CallNonvirtualVoidMethodA(OMElysiaNativeHandle *obj, OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                   const OMElysiaNativeValue *args)
    {
        internal->CallNonvirtualVoidMethodA(this, obj, clazz, methodID, args);
    }
    OMElysiaField *GetFieldID(OMElysiaKlass *clazz, const char *name, const char *sig)
    {
        return internal->GetFieldID(this, clazz, name, sig);
    }
    OMElysiaNativeHandle *GetObjectField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID)
    {
        return internal->GetObjectField(this, obj, fieldID);
    }
    jboolean GetBooleanField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID)
    {
        return internal->GetBooleanField(this, obj, fieldID);
    }
    jbyte GetByteField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID)
    {
        return internal->GetByteField(this, obj, fieldID);
    }
    jchar GetCharField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID)
    {
        return internal->GetCharField(this, obj, fieldID);
    }
    jshort GetShortField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID)
    {
        return internal->GetShortField(this, obj, fieldID);
    }
    jint GetIntField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID)
    {
        return internal->GetIntField(this, obj, fieldID);
    }
    jlong GetLongField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID)
    {
        return internal->GetLongField(this, obj, fieldID);
    }
    jfloat GetFloatField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID)
    {
        return internal->GetFloatField(this, obj, fieldID);
    }
    jdouble GetDoubleField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID)
    {
        return internal->GetDoubleField(this, obj, fieldID);
    }
    void SetObjectField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID, OMElysiaNativeHandle *val)
    {
        internal->SetObjectField(this, obj, fieldID, val);
    }
    void SetBooleanField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jboolean val)
    {
        internal->SetBooleanField(this, obj, fieldID, val);
    }
    void SetByteField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jbyte val)
    {
        internal->SetByteField(this, obj, fieldID, val);
    }
    void SetCharField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jchar val)
    {
        internal->SetCharField(this, obj, fieldID, val);
    }
    void SetShortField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jshort val)
    {
        internal->SetShortField(this, obj, fieldID, val);
    }
    void SetIntField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jint val)
    {
        internal->SetIntField(this, obj, fieldID, val);
    }
    void SetLongField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jlong val)
    {
        internal->SetLongField(this, obj, fieldID, val);
    }
    void SetFloatField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jfloat val)
    {
        internal->SetFloatField(this, obj, fieldID, val);
    }
    void SetDoubleField(OMElysiaNativeHandle *obj, OMElysiaField *fieldID, jdouble val)
    {
        internal->SetDoubleField(this, obj, fieldID, val);
    }
    OMElysiaMethod *GetStaticMethodID(OMElysiaKlass *clazz, const char *name, const char *sig)
    {
        return internal->GetStaticMethodID(this, clazz, name, sig);
    }
    template <typename... Ts>
    OMElysiaNativeHandle *CallStaticObjectMethod(OMElysiaKlass *clazz, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallStaticObjectMethod(this, clazz, methodID, std::forward<Ts>(args)...);
    }
    OMElysiaNativeHandle *CallStaticObjectMethodV(OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallStaticObjectMethodV(this, clazz, methodID, args);
    }
    OMElysiaNativeHandle *CallStaticObjectMethodA(OMElysiaKlass *clazz, OMElysiaMethod *methodID,
                                                  const OMElysiaNativeValue *args)
    {
        return internal->CallStaticObjectMethodA(this, clazz, methodID, args);
    }
    template <typename... Ts>
    jboolean CallStaticBooleanMethod(OMElysiaKlass *clazz, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallStaticBooleanMethod(this, clazz, methodID, std::forward<Ts>(args)...);
    }
    jboolean CallStaticBooleanMethodV(OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallStaticBooleanMethodV(this, clazz, methodID, args);
    }
    jboolean CallStaticBooleanMethodA(OMElysiaKlass *clazz, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallStaticBooleanMethodA(this, clazz, methodID, args);
    }
    template <typename... Ts> jbyte CallStaticByteMethod(OMElysiaKlass *clazz, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallStaticByteMethod(this, clazz, methodID, std::forward<Ts>(args)...);
    }
    jbyte CallStaticByteMethodV(OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallStaticByteMethodV(this, clazz, methodID, args);
    }
    jbyte CallStaticByteMethodA(OMElysiaKlass *clazz, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallStaticByteMethodA(this, clazz, methodID, args);
    }
    template <typename... Ts> jchar CallStaticCharMethod(OMElysiaKlass *clazz, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallStaticCharMethod(this, clazz, methodID, std::forward<Ts>(args)...);
    }
    jchar CallStaticCharMethodV(OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallStaticCharMethodV(this, clazz, methodID, args);
    }
    jchar CallStaticCharMethodA(OMElysiaKlass *clazz, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallStaticCharMethodA(this, clazz, methodID, args);
    }
    template <typename... Ts> jshort CallStaticShortMethod(OMElysiaKlass *clazz, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallStaticShortMethod(this, clazz, methodID, std::forward<Ts>(args)...);
    }
    jshort CallStaticShortMethodV(OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallStaticShortMethodV(this, clazz, methodID, args);
    }
    jshort CallStaticShortMethodA(OMElysiaKlass *clazz, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallStaticShortMethodA(this, clazz, methodID, args);
    }
    template <typename... Ts> jint CallStaticIntMethod(OMElysiaKlass *clazz, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallStaticIntMethod(this, clazz, methodID, std::forward<Ts>(args)...);
    }
    jint CallStaticIntMethodV(OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallStaticIntMethodV(this, clazz, methodID, args);
    }
    jint CallStaticIntMethodA(OMElysiaKlass *clazz, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallStaticIntMethodA(this, clazz, methodID, args);
    }
    template <typename... Ts> jlong CallStaticLongMethod(OMElysiaKlass *clazz, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallStaticLongMethod(this, clazz, methodID, std::forward<Ts>(args)...);
    }
    jlong CallStaticLongMethodV(OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallStaticLongMethodV(this, clazz, methodID, args);
    }
    jlong CallStaticLongMethodA(OMElysiaKlass *clazz, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallStaticLongMethodA(this, clazz, methodID, args);
    }
    template <typename... Ts> jfloat CallStaticFloatMethod(OMElysiaKlass *clazz, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallStaticFloatMethod(this, clazz, methodID, std::forward<Ts>(args)...);
    }
    jfloat CallStaticFloatMethodV(OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallStaticFloatMethodV(this, clazz, methodID, args);
    }
    jfloat CallStaticFloatMethodA(OMElysiaKlass *clazz, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallStaticFloatMethodA(this, clazz, methodID, args);
    }
    template <typename... Ts> jdouble CallStaticDoubleMethod(OMElysiaKlass *clazz, OMElysiaMethod *methodID, Ts... args)
    {
        return internal->CallStaticDoubleMethod(this, clazz, methodID, std::forward<Ts>(args)...);
    }
    jdouble CallStaticDoubleMethodV(OMElysiaKlass *clazz, OMElysiaMethod *methodID, va_list args)
    {
        return internal->CallStaticDoubleMethodV(this, clazz, methodID, args);
    }
    jdouble CallStaticDoubleMethodA(OMElysiaKlass *clazz, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        return internal->CallStaticDoubleMethodA(this, clazz, methodID, args);
    }
    template <typename... Ts> void CallStaticVoidMethod(OMElysiaKlass *cls, OMElysiaMethod *methodID, Ts... args)
    {
        internal->CallStaticVoidMethod(this, cls, methodID, std::forward<Ts>(args)...);
    }
    void CallStaticVoidMethodV(OMElysiaKlass *cls, OMElysiaMethod *methodID, va_list args)
    {
        internal->CallStaticVoidMethodV(this, cls, methodID, args);
    }
    void CallStaticVoidMethodA(OMElysiaKlass *cls, OMElysiaMethod *methodID, const OMElysiaNativeValue *args)
    {
        internal->CallStaticVoidMethodA(this, cls, methodID, args);
    }
    OMElysiaField *GetStaticFieldID(OMElysiaKlass *clazz, const char *name, const char *sig)
    {
        return internal->GetStaticFieldID(this, clazz, name, sig);
    }
    OMElysiaNativeHandle *GetStaticObjectField(OMElysiaKlass *clazz, OMElysiaField *fieldID)
    {
        return internal->GetStaticObjectField(this, clazz, fieldID);
    }
    jboolean GetStaticBooleanField(OMElysiaKlass *clazz, OMElysiaField *fieldID)
    {
        return internal->GetStaticBooleanField(this, clazz, fieldID);
    }
    jbyte GetStaticByteField(OMElysiaKlass *clazz, OMElysiaField *fieldID)
    {
        return internal->GetStaticByteField(this, clazz, fieldID);
    }
    jchar GetStaticCharField(OMElysiaKlass *clazz, OMElysiaField *fieldID)
    {
        return internal->GetStaticCharField(this, clazz, fieldID);
    }
    jshort GetStaticShortField(OMElysiaKlass *clazz, OMElysiaField *fieldID)
    {
        return internal->GetStaticShortField(this, clazz, fieldID);
    }
    jint GetStaticIntField(OMElysiaKlass *clazz, OMElysiaField *fieldID)
    {
        return internal->GetStaticIntField(this, clazz, fieldID);
    }
    jlong GetStaticLongField(OMElysiaKlass *clazz, OMElysiaField *fieldID)
    {
        return internal->GetStaticLongField(this, clazz, fieldID);
    }
    jfloat GetStaticFloatField(OMElysiaKlass *clazz, OMElysiaField *fieldID)
    {
        return internal->GetStaticFloatField(this, clazz, fieldID);
    }
    jdouble GetStaticDoubleField(OMElysiaKlass *clazz, OMElysiaField *fieldID)
    {
        return internal->GetStaticDoubleField(this, clazz, fieldID);
    }
    void SetStaticObjectField(OMElysiaKlass *clazz, OMElysiaField *fieldID, OMElysiaNativeHandle *value)
    {
        internal->SetStaticObjectField(this, clazz, fieldID, value);
    }
    void SetStaticBooleanField(OMElysiaKlass *clazz, OMElysiaField *fieldID, jboolean value)
    {
        internal->SetStaticBooleanField(this, clazz, fieldID, value);
    }
    void SetStaticByteField(OMElysiaKlass *clazz, OMElysiaField *fieldID, jbyte value)
    {
        internal->SetStaticByteField(this, clazz, fieldID, value);
    }
    void SetStaticCharField(OMElysiaKlass *clazz, OMElysiaField *fieldID, jchar value)
    {
        internal->SetStaticCharField(this, clazz, fieldID, value);
    }
    void SetStaticShortField(OMElysiaKlass *clazz, OMElysiaField *fieldID, jshort value)
    {
        internal->SetStaticShortField(this, clazz, fieldID, value);
    }
    void SetStaticIntField(OMElysiaKlass *clazz, OMElysiaField *fieldID, jint value)
    {
        internal->SetStaticIntField(this, clazz, fieldID, value);
    }
    void SetStaticLongField(OMElysiaKlass *clazz, OMElysiaField *fieldID, jlong value)
    {
        internal->SetStaticLongField(this, clazz, fieldID, value);
    }
    void SetStaticFloatField(OMElysiaKlass *clazz, OMElysiaField *fieldID, jfloat value)
    {
        internal->SetStaticFloatField(this, clazz, fieldID, value);
    }
    void SetStaticDoubleField(OMElysiaKlass *clazz, OMElysiaField *fieldID, jdouble value)
    {
        internal->SetStaticDoubleField(this, clazz, fieldID, value);
    }
    OMElysiaNativeHandle *NewString(const jchar *unicode, jsize len)
    {
        return internal->NewString(this, unicode, len);
    }
    jsize GetStringLength(OMElysiaNativeHandle *str)
    {
        return internal->GetStringLength(this, str);
    }
    const jchar *GetStringChars(OMElysiaNativeHandle *str, jboolean *isCopy)
    {
        return internal->GetStringChars(this, str, isCopy);
    }
    void ReleaseStringChars(OMElysiaNativeHandle *str, const jchar *chars)
    {
        internal->ReleaseStringChars(this, str, chars);
    }
    OMElysiaNativeHandle *NewStringUTF(const char *utf)
    {
        return internal->NewStringUTF(this, utf);
    }
    jsize GetStringUTFLength(OMElysiaNativeHandle *str)
    {
        return internal->GetStringUTFLength(this, str);
    }
    const char *GetStringUTFChars(OMElysiaNativeHandle *str, jboolean *isCopy)
    {
        return internal->GetStringUTFChars(this, str, isCopy);
    }
    void ReleaseStringUTFChars(OMElysiaNativeHandle *str, const char *chars)
    {
        internal->ReleaseStringUTFChars(this, str, chars);
    }
    jsize GetArrayLength(OMElysiaNativeHandle *array)
    {
        return internal->GetArrayLength(this, array);
    }
    OMElysiaNativeHandle *NewObjectArray(jsize len, OMElysiaKlass *clazz, OMElysiaNativeHandle *init)
    {
        return internal->NewObjectArray(this, len, clazz, init);
    }
    OMElysiaNativeHandle *GetObjectArrayElement(OMElysiaNativeHandle *array, jsize index)
    {
        return internal->GetObjectArrayElement(this, array, index);
    }
    void SetObjectArrayElement(OMElysiaNativeHandle *array, jsize index, OMElysiaNativeHandle *val)
    {
        internal->SetObjectArrayElement(this, array, index, val);
    }
    OMElysiaNativeHandle *NewBooleanArray(jsize len)
    {
        return internal->NewBooleanArray(this, len);
    }
    OMElysiaNativeHandle *NewByteArray(jsize len)
    {
        return internal->NewByteArray(this, len);
    }
    OMElysiaNativeHandle *NewCharArray(jsize len)
    {
        return internal->NewCharArray(this, len);
    }
    OMElysiaNativeHandle *NewShortArray(jsize len)
    {
        return internal->NewShortArray(this, len);
    }
    OMElysiaNativeHandle *NewIntArray(jsize len)
    {
        return internal->NewIntArray(this, len);
    }
    OMElysiaNativeHandle *NewLongArray(jsize len)
    {
        return internal->NewLongArray(this, len);
    }
    OMElysiaNativeHandle *NewFloatArray(jsize len)
    {
        return internal->NewFloatArray(this, len);
    }
    OMElysiaNativeHandle *NewDoubleArray(jsize len)
    {
        return internal->NewDoubleArray(this, len);
    }
    jboolean *GetBooleanArrayElements(OMElysiaNativeHandle *array, jboolean *isCopy)
    {
        return internal->GetBooleanArrayElements(this, array, isCopy);
    }
    jbyte *GetByteArrayElements(OMElysiaNativeHandle *array, jboolean *isCopy)
    {
        return internal->GetByteArrayElements(this, array, isCopy);
    }
    jchar *GetCharArrayElements(OMElysiaNativeHandle *array, jboolean *isCopy)
    {
        return internal->GetCharArrayElements(this, array, isCopy);
    }
    jshort *GetShortArrayElements(OMElysiaNativeHandle *array, jboolean *isCopy)
    {
        return internal->GetShortArrayElements(this, array, isCopy);
    }
    jint *GetIntArrayElements(OMElysiaNativeHandle *array, jboolean *isCopy)
    {
        return internal->GetIntArrayElements(this, array, isCopy);
    }
    jlong *GetLongArrayElements(OMElysiaNativeHandle *array, jboolean *isCopy)
    {
        return internal->GetLongArrayElements(this, array, isCopy);
    }
    jfloat *GetFloatArrayElements(OMElysiaNativeHandle *array, jboolean *isCopy)
    {
        return internal->GetFloatArrayElements(this, array, isCopy);
    }
    jdouble *GetDoubleArrayElements(OMElysiaNativeHandle *array, jboolean *isCopy)
    {
        return internal->GetDoubleArrayElements(this, array, isCopy);
    }
    void ReleaseBooleanArrayElements(OMElysiaNativeHandle *array, jboolean *elems, jint mode)
    {
        internal->ReleaseBooleanArrayElements(this, array, elems, mode);
    }
    void ReleaseByteArrayElements(OMElysiaNativeHandle *array, jbyte *elems, jint mode)
    {
        internal->ReleaseByteArrayElements(this, array, elems, mode);
    }
    void ReleaseCharArrayElements(OMElysiaNativeHandle *array, jchar *elems, jint mode)
    {
        internal->ReleaseCharArrayElements(this, array, elems, mode);
    }
    void ReleaseShortArrayElements(OMElysiaNativeHandle *array, jshort *elems, jint mode)
    {
        internal->ReleaseShortArrayElements(this, array, elems, mode);
    }
    void ReleaseIntArrayElements(OMElysiaNativeHandle *array, jint *elems, jint mode)
    {
        internal->ReleaseIntArrayElements(this, array, elems, mode);
    }
    void ReleaseLongArrayElements(OMElysiaNativeHandle *array, jlong *elems, jint mode)
    {
        internal->ReleaseLongArrayElements(this, array, elems, mode);
    }
    void ReleaseFloatArrayElements(OMElysiaNativeHandle *array, jfloat *elems, jint mode)
    {
        internal->ReleaseFloatArrayElements(this, array, elems, mode);
    }
    void ReleaseDoubleArrayElements(OMElysiaNativeHandle *array, jdouble *elems, jint mode)
    {
        internal->ReleaseDoubleArrayElements(this, array, elems, mode);
    }
    void GetBooleanArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, jboolean *buf)
    {
        internal->GetBooleanArrayRegion(this, array, start, len, buf);
    }
    void GetByteArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, jbyte *buf)
    {
        internal->GetByteArrayRegion(this, array, start, len, buf);
    }
    void GetCharArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, jchar *buf)
    {
        internal->GetCharArrayRegion(this, array, start, len, buf);
    }
    void GetShortArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, jshort *buf)
    {
        internal->GetShortArrayRegion(this, array, start, len, buf);
    }
    void GetIntArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, jint *buf)
    {
        internal->GetIntArrayRegion(this, array, start, len, buf);
    }
    void GetLongArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, jlong *buf)
    {
        internal->GetLongArrayRegion(this, array, start, len, buf);
    }
    void GetFloatArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, jfloat *buf)
    {
        internal->GetFloatArrayRegion(this, array, start, len, buf);
    }
    void GetDoubleArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, jdouble *buf)
    {
        internal->GetDoubleArrayRegion(this, array, start, len, buf);
    }
    void SetBooleanArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, const jboolean *buf)
    {
        internal->SetBooleanArrayRegion(this, array, start, len, buf);
    }
    void SetByteArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, const jbyte *buf)
    {
        internal->SetByteArrayRegion(this, array, start, len, buf);
    }
    void SetCharArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, const jchar *buf)
    {
        internal->SetCharArrayRegion(this, array, start, len, buf);
    }
    void SetShortArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, const jshort *buf)
    {
        internal->SetShortArrayRegion(this, array, start, len, buf);
    }
    void SetIntArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, const jint *buf)
    {
        internal->SetIntArrayRegion(this, array, start, len, buf);
    }
    void SetLongArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, const jlong *buf)
    {
        internal->SetLongArrayRegion(this, array, start, len, buf);
    }
    void SetFloatArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, const jfloat *buf)
    {
        internal->SetFloatArrayRegion(this, array, start, len, buf);
    }
    void SetDoubleArrayRegion(OMElysiaNativeHandle *array, jsize start, jsize len, const jdouble *buf)
    {
        internal->SetDoubleArrayRegion(this, array, start, len, buf);
    }
    jint RegisterNatives(OMElysiaKlass *clazz, const OMElysiaNativeMethod *methods, jint nMethods)
    {
        return internal->RegisterNatives(this, clazz, methods, nMethods);
    }
    jint UnregisterNatives(OMElysiaKlass *clazz)
    {
        return internal->UnregisterNatives(this, clazz);
    }
    jint MonitorEnter(OMElysiaNativeHandle *obj)
    {
        return internal->MonitorEnter(this, obj);
    }
    jint MonitorExit(OMElysiaNativeHandle *obj)
    {
        return internal->MonitorExit(this, obj);
    }
    jint GetJavaVM(void **vm)
    {
        return internal->GetJavaVM(this, vm);
    }
    void GetStringRegion(OMElysiaNativeHandle *str, jsize start, jsize len, jchar *buf)
    {
        internal->GetStringRegion(this, str, start, len, buf);
    }
    void GetStringUTFRegion(OMElysiaNativeHandle *str, jsize start, jsize len, char *buf)
    {
        internal->GetStringUTFRegion(this, str, start, len, buf);
    }
    void *GetPrimitiveArrayCritical(OMElysiaNativeHandle *array, jboolean *isCopy)
    {
        return internal->GetPrimitiveArrayCritical(this, array, isCopy);
    }
    void ReleasePrimitiveArrayCritical(OMElysiaNativeHandle *array, void *carray, jint mode)
    {
        internal->ReleasePrimitiveArrayCritical(this, array, carray, mode);
    }
    const jchar *GetStringCritical(OMElysiaNativeHandle *string, jboolean *isCopy)
    {
        return internal->GetStringCritical(this, string, isCopy);
    }
    void ReleaseStringCritical(OMElysiaNativeHandle *string, const jchar *cstring)
    {
        internal->ReleaseStringCritical(this, string, cstring);
    }
    OMElysiaNativeHandle *NewWeakGlobalRef(OMElysiaNativeHandle *obj)
    {
        return internal->NewWeakGlobalRef(this, obj);
    }
    void DeleteWeakGlobalRef(OMElysiaNativeHandle *ref)
    {
        internal->DeleteWeakGlobalRef(this, ref);
    }
    jboolean ExceptionCheck()
    {
        return internal->ExceptionCheck(this);
    }
    OMElysiaNativeHandle *NewDirectByteBuffer(void *address, jlong capacity)
    {
        return internal->NewDirectByteBuffer(this, address, capacity);
    }
    void *GetDirectBufferAddress(OMElysiaNativeHandle *buf)
    {
        return internal->GetDirectBufferAddress(this, buf);
    }
    jlong GetDirectBufferCapacity(OMElysiaNativeHandle *buf)
    {
        return internal->GetDirectBufferCapacity(this, buf);
    }
    jint GetObjectRefType(OMElysiaNativeHandle *obj)
    {
        return internal->GetObjectRefType(this, obj);
    }
    OMElysiaNativeHandle *GetModule(OMElysiaKlass *clazz)
    {
        return internal->GetModule(this, clazz);
    }
    jboolean IsVirtualThread(OMElysiaNativeHandle *obj)
    {
        return internal->IsVirtualThread(this, obj);
    }
};
#endif
} // namespace openminecraft::vm::elysia

#endif
