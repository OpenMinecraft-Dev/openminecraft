#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/os/om_cpuname.hpp"
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <pwd.h>
#include <stdlib.h>
#include <string>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <unordered_map>

extern "C"
{
    void cpuinfo_x86(uint32_t op, int32_t *eax, int32_t *ebx, int32_t *ecx, int32_t *edx);
    uint64_t cpuinfo_aarch64();
}

namespace openminecraft::vm::os
{
static std::unordered_map<std::string, std::string> fetchConfig()
{
    std::ifstream f("/etc/os-release");
    std::string staging;
    std::unordered_map<std::string, std::string> result;
    while (f.good())
    {
        char d;
        f.read(&d, 1);
        staging += d;
    }

    f.close();

    char data[64] = {0};
    if (sscanf(staging.c_str(), "NAME=\"%[^\"]\"\n", data))
    {
        result["name"] = std::string(data);
    }
    if (sscanf(staging.c_str(), "BUILD_ID=\"%[^\"]\"\n", data))
    {
        result["version"] = std::string(data);
    }
    if (sscanf(staging.c_str(), "VERSION=\"%[^\"]\"\n", data))
    {
        result["version"] = std::string(data);
    }

    return result;
}
void fetchFromDevFs()
{
}
std::string fetchCpuName()
{
#if defined(__x86_64__) || defined(__x86__)
    char model[64] = {0};
    int32_t *d = (int32_t *)model;
    cpuinfo_x86(0x80000002, &d[0], &d[1], &d[2], &d[3]);
    cpuinfo_x86(0x80000003, &d[4], &d[5], &d[6], &d[7]);
    cpuinfo_x86(0x80000004, &d[8], &d[9], &d[10], &d[11]);
    model[48] = '\0';
    return std::string(model);
#else
    return fmt::format("unknown {} cpu", n.machine);
#endif
}
std::string fetchUsername()
{
    return std::string(getpwuid(getuid())->pw_name);
}
std::string fetchLoginUser()
{
    return std::string(getlogin());
}
std::string fetchSystemName()
{
    auto f = fetchConfig();
    if (f["name"].size())
    {
        return f["name"];
    }
    struct utsname n;
    uname(&n);
    return std::string(n.sysname);
}
std::string fetchSystemVersion()
{
    auto f = fetchConfig();
    if (f["version"].size())
    {
        return f["version"];
    }
    struct utsname n;
    uname(&n);
    return std::string(n.release);
}
uint64_t fetchMemoryTotal()
{
    return sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
}
uint64_t fetchMemoryAvailable()
{
    return sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE);
}
} // namespace openminecraft::vm::os