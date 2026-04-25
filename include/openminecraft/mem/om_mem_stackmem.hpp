#ifndef OM_MEM_STACKMEM_HPP
#define OM_MEM_STACKMEM_HPP

#include <cstdint>
#include <thread>
namespace openminecraft::mem::stack
{
uintptr_t fetchStackBase(std::thread thr);
uintptr_t fetchStackTop(std::thread thr);
}; // namespace openminecraft::mem::stack

#endif
