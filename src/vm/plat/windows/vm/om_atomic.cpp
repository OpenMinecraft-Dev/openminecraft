#include <intrin.h>

namespace openminecraft::vm::atomic
{
int atomic_cas(int *addr, int expected, int desired)
{
    return _InterlockedCompareExchange(reinterpret_cast<volatile long *>(addr), expected, desired);
}

int atomic_load(int *addr)
{
    return _InterlockedOr(reinterpret_cast<volatile long *>(addr), 0);
}

void atomic_store(int *addr, int value)
{
    _InterlockedExchange(reinterpret_cast<volatile long *>(addr), value);
}
}
