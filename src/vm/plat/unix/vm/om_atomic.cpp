namespace openminecraft::vm::atomic
{
int atomic_cas(int *addr, int expected, int desired)
{
    __atomic_compare_exchange_n(addr, &expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return expected;
}
int atomic_load(int *addr)
{
    return __atomic_load_n(addr, __ATOMIC_SEQ_CST);
}
void atomic_store(int *addr, int value)
{
    __atomic_store_n(addr, value, __ATOMIC_SEQ_CST);
}
} // namespace openminecraft::vm::atomic
