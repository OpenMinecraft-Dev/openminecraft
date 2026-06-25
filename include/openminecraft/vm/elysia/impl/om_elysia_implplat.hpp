#ifndef OM_ELYSIA_IMPLPLAT_HPP
#define OM_ELYSIA_IMPLPLAT_HPP

#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include <cstdint>
namespace openminecraft::vm::elysia::impl
{
uint64_t getNativeFd(OMElysiaJNIEnv *env, OMElysiaNativeHandle *fd);
}

#endif
