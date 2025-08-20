#include "openminecraft/vm/os/om_cpuname.hpp"
#include <cstdint>
#include <iostream>
#include <sys/utsname.h>

extern "C"
{
    void cpuinfo_x86(uint32_t op, int32_t *eax, int32_t *ebx, int32_t *ecx, int32_t *edx);
}

namespace openminecraft::vm::os
{
std::string fetchCpuName()
{
    struct utsname n;
    uname(&n);
    std::cout << n.machine << std::endl;
    std::cout << n.nodename << std::endl;
    std::cout << n.release << std::endl;
    std::cout << n.sysname << std::endl;
    std::cout << n.version << std::endl;

#if defined(__x86_64__) || defined(__x86__)
    char model[64] = {0};
    int32_t *d = (int32_t *)model;
    cpuinfo_x86(0x80000002, &d[0], &d[1], &d[2], &d[3]);
    cpuinfo_x86(0x80000003, &d[4], &d[5], &d[6], &d[7]);
    cpuinfo_x86(0x80000004, &d[8], &d[9], &d[10], &d[11]);
    model[48] = '\0';
    return std::string(model);
#else
    return "";
#endif
}
} // namespace openminecraft::vm::os