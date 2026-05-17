#ifndef OM_ATOMIC_HPP
#define OM_ATOMIC_HPP

namespace openminecraft::vm::atomic
{
int atomic_cas(int *addr, int expected, int desired);
int atomic_load(int *addr);
void atomic_store(int *addr, int value);
} // namespace openminecraft::vm::atomic

#endif
