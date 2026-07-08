#ifndef OM_ELYSIA_INTERFACE_UTILS
#define OM_ELYSIA_INTERFACE_UTILS

#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_field.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include <initializer_list>
#include <type_traits>

namespace openminecraft::vm::elysia::interface
{
struct OMElysiaNativeMethodWrapper
{
    const char *name;
    const char *signature;
    void *func;

    OMElysiaNativeMethodWrapper() = default;

    template <typename Fp>
    OMElysiaNativeMethodWrapper(const char *n, const char *s, Fp f)
        : name(n), signature(s), func(reinterpret_cast<void *>(f))
    {
        static_assert(std::is_pointer_v<Fp>, "only function pointers");
    }
};

static inline void registerNativeFuncs(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                       std::initializer_list<OMElysiaNativeMethodWrapper> funcs)
{
    auto cnt = funcs.size();
    auto funcstarget = reinterpret_cast<OMElysiaNativeMethod *>(
        mem::allocator::tracedMallocElysia(cnt * sizeof(OMElysiaNativeMethod)));

    int i = 0;
    for (const auto &func : funcs)
    {
        funcstarget[i].name = const_cast<char *>(func.name);
        funcstarget[i].signature = const_cast<char *>(func.signature);
        funcstarget[i].funcPtr = func.func;
        ++i;
    }
    env->RegisterNatives(klass, funcstarget, cnt);
    mem::allocator::tracedFreeElysia(funcstarget);
}

static inline auto field(OMElysiaJNIEnv *env, std::string cname, std::string fname, std::string fdesc)
    -> OMElysiaField *
{
    return env->GetFieldID(env->FindClass(cname.c_str()), fname.c_str(), fdesc.c_str());
}

static inline auto method(OMElysiaJNIEnv *env, std::string cname, std::string mname, std::string mdesc)
    -> OMElysiaMethod *
{
    return env->GetMethodID(env->FindClass(cname.c_str()), mname.c_str(), mdesc.c_str());
}
static inline auto staticMethod(OMElysiaJNIEnv *env, std::string cname, std::string mname, std::string mdesc)
    -> OMElysiaMethod *
{
    return env->GetStaticMethodID(env->FindClass(cname.c_str()), mname.c_str(), mdesc.c_str());
}

} // namespace openminecraft::vm::elysia::interface

#endif
