#include "openminecraft/vm/os/om_hardware.hpp"
#include <array>
#include <cstdint>
#include <fmt/format.h>
#include <fstream>
#include <pwd.h>
#include <sstream>
#include <string>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <unordered_map>
#include <variant>

extern "C"
{
    void cpuinfo_x86(uint32_t op, int32_t *eax, int32_t *ebx, int32_t *ecx, int32_t *edx);
    auto cpuinfo_aarch64() -> uint64_t;
    void aarch64_tocpuname(uint64_t d, std::string *ss);
}

namespace openminecraft::vm::os
{
using std::string;

static auto fetchConfig() -> std::unordered_map<std::string, std::string>
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

    std::istringstream ss(staging);
    std::string n;
    while (std::getline(ss, n, '\n'))
    {
        if (n.find("NAME=") != std::variant_npos && !result["name"].size())
        {
            result["name"] = n.substr(5);
        }
        else if (n.find("BUILD_ID=") != std::variant_npos && !result["version"].size())
        {
            result["version"] = n.substr(9);
        }
        else if (n.find("VERSION=") != std::variant_npos && !result["version"].size())
        {
            result["version"] = n.substr(8);
        }
    }

    while (result["name"].find("\"") != std::variant_npos)
    {
        result["name"].erase(result["name"].find("\""), 1);
    }
    while (result["version"].find("\"") != std::variant_npos)
    {
        result["version"].erase(result["version"].find("\""), 1);
    }

    return result;
}
static auto fetchFromDevFs() -> std::string
{
    std::ifstream f("/proc/cpuinfo");
    std::string staging;
    while (f.good())
    {
        char d;
        f.read(&d, 1);
        staging += d;
    }

    f.close();
    auto d = staging.find("model name");
    if (d == std::variant_npos)
    {
        d = staging.find("Model Name");
    }
    if (d != std::variant_npos)
    {
        auto d2 = staging.find("\n", d + 10);
        auto temp = std::string(staging.c_str()).substr(d + 10, d2 - d - 10);
        auto dt = temp.find(":");
        return temp.substr(dt + 2);
    }
    return "";
}
auto fetchCpuName() -> std::string
{
    utsname n{};
    uname(&n);
#if defined(__x86_64__) || defined(__x86__)
    std::array<char, 64> model;
    auto *d = reinterpret_cast<int32_t *>(model.data());
    cpuinfo_x86(0x80000002, &d[0], &d[1], &d[2], &d[3]);
    cpuinfo_x86(0x80000003, &d[4], &d[5], &d[6], &d[7]);
    cpuinfo_x86(0x80000004, &d[8], &d[9], &d[10], &d[11]);
    model[48] = '\0';
    auto st = std::string(model.data());
    std::string result;
    for (auto ch : st)
    {
        if (ch != ' ' || result[result.size() - 1] != ' ')
        {
            result += ch;
        }
    }
    return result;
#elif defined(__aarch64__)
    std::string cpuname;
    aarch64_tocpuname(cpuinfo_aarch64(), &cpuname);
    return cpuname;
#else
    auto st = fetchFromDevFs();
    return st == "" ? fmt::format("unknown {} cpu", n.machine) : st;
#endif
}
auto fetchUsername() -> std::string
{
    return getpwuid(getuid())->pw_name;
}
auto fetchLoginUser() -> std::string
{
    if (getlogin())
    {
        return getlogin();
    }
    else
    {
        return "unknown";
    }
}
auto fetchSystemName() -> std::string
{
    auto f = fetchConfig();
    if (f["name"].size())
    {
        return f["name"];
    }
    struct utsname n;
    uname(&n);
    return n.sysname;
}
auto fetchSystemVersion() -> std::string
{
    auto f = fetchConfig();
    if (f["version"].size())
    {
        return f["version"];
    }
    struct utsname n;
    uname(&n);
    return n.release;
}
auto fetchMemoryTotal() -> uint64_t
{
    return sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
}
auto fetchAvailableProcessors() -> uint64_t
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}
auto fetchLoadAverage(double *buf, int siz) -> int
{
#ifndef OM_PLATFORM_ANDROID
    return getloadavg(buf, siz);
#else
    return -1;
#endif
}
auto fetchPageSize() -> uint64_t
{
    return sysconf(_SC_PAGE_SIZE);
}
} // namespace openminecraft::vm::os
