#ifndef OM_MEM_STACKMEM_HPP
#define OM_MEM_STACKMEM_HPP

#include <cstdint>
namespace openminecraft::mem::stack
{
uintptr_t fetchStackBase();
uintptr_t fetchStackTop();
}; // namespace openminecraft::mem::stack

#endif
