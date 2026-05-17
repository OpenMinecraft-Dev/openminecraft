#ifndef OM_ELYSIA_IMPLBASE_HPP
#define OM_ELYSIA_IMPLBASE_HPP

#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
namespace openminecraft::vm::elysia
{
class OMElysiaVirtualWorld;
};

namespace openminecraft::vm::elysia::impl
{
extern "C"
{
    void Java_java_lang_System_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass);
    void Java_java_lang_Object_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass);
    void Java_java_lang_Class_registerNatives(OMElysiaJNIEnv *env, OMElysiaKlass *klass);

    jint Java_java_lang_Float_floatToRawIntBits(OMElysiaJNIEnv *env, OMElysiaKlass *klass, jfloat f);
}
}; // namespace openminecraft::vm::elysia::impl

#endif
