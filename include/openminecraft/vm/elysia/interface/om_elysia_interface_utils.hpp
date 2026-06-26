#ifndef OM_ELYSIA_INTERFACE_UTILS
#define OM_ELYSIA_INTERFACE_UTILS

#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
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

static void registerNativeFuncs(OMElysiaJNIEnv *env, OMElysiaKlass *klass,
                                std::initializer_list<OMElysiaNativeMethodWrapper> funcs)
{
    auto cnt = funcs.size();
    auto funcstarget = reinterpret_cast<OMElysiaNativeMethod *>(
        mem::allocator::tracedMallocElysia(cnt * sizeof(OMElysiaNativeMethod)));

    int i = 0;
    for (auto itt = funcs.begin(); itt != funcs.end(); ++itt)
    {
        funcstarget[i].name = const_cast<char *>(itt->name);
        funcstarget[i].signature = const_cast<char *>(itt->signature);
        funcstarget[i].funcPtr = itt->func;
        ++i;
    }
    env->RegisterNatives(klass, funcstarget, cnt);
    mem::allocator::tracedFreeElysia(funcstarget);
}
} // namespace openminecraft::vm::elysia::interface

#endif
