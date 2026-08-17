#include "fmt/format.h"
#include "openminecraft/vm/os/om_hardware.hpp"
#include "windows.h"
#include <sysinfoapi.h>

namespace openminecraft::vm::os
{
std::string fetchCpuName()
{
    DWORD length = 256;
    char name[256] = {0};
    RegGetValue(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString", REG_SZ,
                nullptr, name, &length);
    return name;
}
std::string fetchUsername()
{
    char username[1024];
    DWORD length = 1024;
    GetUserName(username, &length);
    return username;
}
std::string fetchLoginUser()
{
    char username[1024];
    GetEnvironmentVariable("USERNAME", username, 1024);
    return username;
}
std::string fetchSystemName()
{
    return "Windows";
}
std::string fetchSystemVersion()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    OSVERSIONINFOEX os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (GetVersionEx((OSVERSIONINFO *)&os))
    {
        switch (os.dwMajorVersion)
        {
        case 4:
            switch (os.dwMinorVersion)
            {
            case 0:
                if (os.dwPlatformId == VER_PLATFORM_WIN32_NT)
                {
                    return "NT 4.0";
                }
                else if (os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
                {
                    return "95";
                }
                else
                {
                    return "NT 4.0";
                }
            case 10:
                return "98";
            case 90:
                return "ME";
            }

        case 5:
            switch (os.dwMinorVersion)
            {
            case 0:
                return "2000";
            case 1:
                return "XP";
            case 2:
                if (os.wProductType == VER_NT_WORKSTATION &&
                    info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
                {
                    return "XP Professional x64 Edition";
                }
                else if (GetSystemMetrics(SM_SERVERR2) == 0)
                {
                    return "Server 2003";
                }
                else if (GetSystemMetrics(SM_SERVERR2) != 0)
                {
                    return "Server 2003 R2";
                }
            }

        case 6:
            switch (os.dwMinorVersion)
            {
            case 0:
                if (os.wProductType == VER_NT_WORKSTATION)
                {
                    return "Vista";
                }
                else
                {
                    return "Server 2008";
                }
            case 1:
                if (os.wProductType == VER_NT_WORKSTATION)
                {
                    return "7";
                }
                else
                {
                    return "Server 2008 R2";
                }
            case 2:
                if (os.wProductType == VER_NT_WORKSTATION)
                {
                    return "8";
                }
                else
                {
                    return "Server 2012";
                }
            case 3:
                if (os.wProductType == VER_NT_WORKSTATION)
                {
                    return "8.1";
                }
                else
                {
                    return "Server 2012 R2";
                }
            }

        case 10:
            switch (os.dwMinorVersion)
            {
            case 0:
                if (os.wProductType == VER_NT_WORKSTATION)
                {
                    return "10";
                }
                else
                {
                    return "Server 2016";
                }
            }

        case 11:
            switch (os.dwMinorVersion)
            {
            case 0:
                if (os.wProductType == VER_NT_WORKSTATION)
                {
                    return "11";
                }
                else
                {
                    return "Server 2022";
                }
            }
        }

        return fmt::format("{}.{} (Build {})", os.dwMajorVersion, os.dwMinorVersion, os.dwBuildNumber);
    }
    return "unknown";
}

uint64_t fetchMemoryTotal()
{
    MEMORYSTATUS status;
    GlobalMemoryStatus(&status);
    return status.dwTotalPhys;
}

uint64_t fetchAvailableProcessors()
{
    SYSTEM_INFO si;
    memset(&si, 0, sizeof(SYSTEM_INFO));
    GetSystemInfo(&si);

    return si.dwNumberOfProcessors;
}

std::string fetchCPUArch()
{
    SYSTEM_INFO si;
    memset(&si, 0, sizeof(SYSTEM_INFO));
    GetSystemInfo(&si);

    switch (si.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_AMD64:
        return "amd64";
    case PROCESSOR_ARCHITECTURE_ARM64:
        return "aarch64";
    case PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64:
    case PROCESSOR_ARCHITECTURE_ARM:
        return "arm32";
    case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
    case PROCESSOR_ARCHITECTURE_IA32_ON_ARM64:
    case PROCESSOR_ARCHITECTURE_INTEL:
        return "x86";
    case PROCESSOR_ARCHITECTURE_IA64:
        return "ia64";
    case PROCESSOR_ARCHITECTURE_MIPS:
        return "mips";
    case PROCESSOR_ARCHITECTURE_PPC:
        return "powerpc";
    default:
        return "amd64";
    }
}
int fetchLoadAverage(double *buf, int siz)
{
    return -1;
}
uint64_t fetchPageSize()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    return si.dwPageSize;
}
} // namespace openminecraft::vm::os
