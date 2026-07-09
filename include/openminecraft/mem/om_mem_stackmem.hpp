#ifndef OM_MEM_STACKMEM_HPP
#define OM_MEM_STACKMEM_HPP

#include <cstdint>
namespace openminecraft::mem::stack
{
auto fetchStackBase() -> uintptr_t;
auto fetchStackTop() -> uintptr_t;
}; // namespace openminecraft::mem::stack

#endif
