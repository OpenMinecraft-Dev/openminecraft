#ifndef OM_ATOMIC_HPP
#define OM_ATOMIC_HPP

#include <atomic>
namespace openminecraft::vm::atomic
{
template <typename T> bool atomic_cas(T *target, T &expected, T x)
{
    return reinterpret_cast<std::atomic<T> *>(target)->compare_exchange_strong(expected, x);
}
template <typename T> T atomic_load(T *target)
{
    return reinterpret_cast<std::atomic<T> *>(target)->load();
}
template <typename T> void atomic_store(T *target, T x)
{
    return reinterpret_cast<std::atomic<T> *>(target)->store(x);
}
} // namespace openminecraft::vm::atomic

#endif
