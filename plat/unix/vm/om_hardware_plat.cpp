#include "fmt/format.h"
#include "openminecraft/vm/os/om_hardware.hpp"
#include <cstdint>
#include <fstream>
#include <pwd.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <unordered_map>
#include <variant>

extern "C"
{
    void cpuinfo_x86(uint32_t op, int32_t *eax, int32_t *ebx, int32_t *ecx, int32_t *edx);
    uint64_t cpuinfo_aarch64();
    uint64_t cpuinfo_arm();
}

struct id_part
{
    const int id;
    const char *name;
};

// gino: copied from https://github.com/util-linux/util-linux/blob/master/sys-utils/lscpu-arm.c for arm device checking
static const struct id_part arm_part[] = {
    {0x810, "ARM810"},        {0x920, "ARM920"},       {0x922, "ARM922"},       {0x926, "ARM926"},
    {0x940, "ARM940"},        {0x946, "ARM946"},       {0x966, "ARM966"},       {0xa20, "ARM1020"},
    {0xa22, "ARM1022"},       {0xa26, "ARM1026"},      {0xb02, "ARM11 MPCore"}, {0xb36, "ARM1136"},
    {0xb56, "ARM1156"},       {0xb76, "ARM1176"},      {0xc05, "Cortex-A5"},    {0xc07, "Cortex-A7"},
    {0xc08, "Cortex-A8"},     {0xc09, "Cortex-A9"},    {0xc0d, "Cortex-A17"}, /* Originally A12 */
    {0xc0f, "Cortex-A15"},    {0xc0e, "Cortex-A17"},   {0xc14, "Cortex-R4"},    {0xc15, "Cortex-R5"},
    {0xc17, "Cortex-R7"},     {0xc18, "Cortex-R8"},    {0xc20, "Cortex-M0"},    {0xc21, "Cortex-M1"},
    {0xc23, "Cortex-M3"},     {0xc24, "Cortex-M4"},    {0xc27, "Cortex-M7"},    {0xc60, "Cortex-M0+"},
    {0xd01, "Cortex-A32"},    {0xd02, "Cortex-A34"},   {0xd03, "Cortex-A53"},   {0xd04, "Cortex-A35"},
    {0xd05, "Cortex-A55"},    {0xd06, "Cortex-A65"},   {0xd07, "Cortex-A57"},   {0xd08, "Cortex-A72"},
    {0xd09, "Cortex-A73"},    {0xd0a, "Cortex-A75"},   {0xd0b, "Cortex-A76"},   {0xd0c, "Neoverse-N1"},
    {0xd0d, "Cortex-A77"},    {0xd0e, "Cortex-A76AE"}, {0xd13, "Cortex-R52"},   {0xd15, "Cortex-R82"},
    {0xd16, "Cortex-R52+"},   {0xd20, "Cortex-M23"},   {0xd21, "Cortex-M33"},   {0xd22, "Cortex-M55"},
    {0xd23, "Cortex-M85"},    {0xd40, "Neoverse-V1"},  {0xd41, "Cortex-A78"},   {0xd42, "Cortex-A78AE"},
    {0xd43, "Cortex-A65AE"},  {0xd44, "Cortex-X1"},    {0xd46, "Cortex-A510"},  {0xd47, "Cortex-A710"},
    {0xd48, "Cortex-X2"},     {0xd49, "Neoverse-N2"},  {0xd4a, "Neoverse-E1"},  {0xd4b, "Cortex-A78C"},
    {0xd4c, "Cortex-X1C"},    {0xd4d, "Cortex-A715"},  {0xd4e, "Cortex-X3"},    {0xd4f, "Neoverse-V2"},
    {0xd80, "Cortex-A520"},   {0xd81, "Cortex-A720"},  {0xd82, "Cortex-X4"},    {0xd83, "Neoverse-V3AE"},
    {0xd84, "Neoverse-V3"},   {0xd85, "Cortex-X925"},  {0xd87, "Cortex-A725"},  {0xd88, "Cortex-A520AE"},
    {0xd89, "Cortex-A720AE"}, {0xd8e, "Neoverse-N3"},  {0xd8f, "Cortex-A320"},  {-1, "unknown"},
};

static const struct id_part brcm_part[] = {
    {0x0f, "Brahma-B15"},
    {0x100, "Brahma-B53"},
    {0x516, "ThunderX2"},
    {-1, "unknown"},
};

static const struct id_part dec_part[] = {
    {0xa10, "SA110"},
    {0xa11, "SA1100"},
    {-1, "unknown"},
};

static const struct id_part cavium_part[] = {
    {0x0a0, "ThunderX"},         {0x0a1, "ThunderX-88XX"},
    {0x0a2, "ThunderX-81XX"},    {0x0a3, "ThunderX-83XX"},
    {0x0af, "ThunderX2-99xx"},   {0x0b0, "OcteonTX2"},
    {0x0b1, "OcteonTX2-98XX"},   {0x0b2, "OcteonTX2-96XX"},
    {0x0b3, "OcteonTX2-95XX"},   {0x0b4, "OcteonTX2-95XXN"},
    {0x0b5, "OcteonTX2-95XXMM"}, {0x0b6, "OcteonTX2-95XXO"},
    {0x0b8, "ThunderX3-T110"},   {-1, "unknown"},
};

static const struct id_part apm_part[] = {
    {0x000, "X-Gene"},
    {-1, "unknown"},
};

static const struct id_part qcom_part[] = {
    {0x001, "Oryon"},          {0x00f, "Scorpion"},
    {0x02d, "Scorpion"},       {0x04d, "Krait"},
    {0x06f, "Krait"},          {0x201, "Kryo"},
    {0x205, "Kryo"},           {0x211, "Kryo"},
    {0x800, "Falkor-V1/Kryo"}, {0x801, "Kryo-V2"},
    {0x802, "Kryo-3XX-Gold"},  {0x803, "Kryo-3XX-Silver"},
    {0x804, "Kryo-4XX-Gold"},  {0x805, "Kryo-4XX-Silver"},
    {0xc00, "Falkor"},         {0xc01, "Saphira"},
    {-1, "unknown"},
};

static const struct id_part samsung_part[] = {
    {0x001, "exynos-m1"}, {0x002, "exynos-m3"}, {0x003, "exynos-m4"}, {0x004, "exynos-m5"}, {-1, "unknown"},
};

static const struct id_part nvidia_part[] = {
    {0x000, "Denver"},
    {0x003, "Denver 2"},
    {0x004, "Carmel"},
    {-1, "unknown"},
};

static const struct id_part marvell_part[] = {
    {0x131, "Feroceon-88FR131"},
    {0x581, "PJ4/PJ4b"},
    {0x584, "PJ4B-MP"},
    {-1, "unknown"},
};

static const struct id_part apple_part[] = {
    {0x000, "Swift"},
    {0x001, "Cyclone"},
    {0x002, "Typhoon"},
    {0x003, "Typhoon/Capri"},
    {0x004, "Twister"},
    {0x005, "Twister/Elba/Malta"},
    {0x006, "Hurricane"},
    {0x007, "Hurricane/Myst"},
    {0x008, "Monsoon"},
    {0x009, "Mistral"},
    {0x00b, "Vortex"},
    {0x00c, "Tempest"},
    {0x00f, "Tempest-M9"},
    {0x010, "Vortex/Aruba"},
    {0x011, "Tempest/Aruba"},
    {0x012, "Lightning"},
    {0x013, "Thunder"},
    {0x020, "Icestorm-A14"},
    {0x021, "Firestorm-A14"},
    {0x022, "Icestorm-M1"},
    {0x023, "Firestorm-M1"},
    {0x024, "Icestorm-M1-Pro"},
    {0x025, "Firestorm-M1-Pro"},
    {0x026, "Thunder-M10"},
    {0x028, "Icestorm-M1-Max"},
    {0x029, "Firestorm-M1-Max"},
    {0x030, "Blizzard-A15"},
    {0x031, "Avalanche-A15"},
    {0x032, "Blizzard-M2"},
    {0x033, "Avalanche-M2"},
    {0x034, "Blizzard-M2-Pro"},
    {0x035, "Avalanche-M2-Pro"},
    {0x036, "Sawtooth-A16"},
    {0x037, "Everest-A16"},
    {0x038, "Blizzard-M2-Max"},
    {0x039, "Avalanche-M2-Max"},
    {-1, "unknown"},
};

static const struct id_part faraday_part[] = {
    {0x526, "FA526"},
    {0x626, "FA626"},
    {-1, "unknown"},
};

static const struct id_part intel_part[] = {
    {0x200, "i80200"},        {0x210, "PXA250A"},
    {0x212, "PXA210A"},       {0x242, "i80321-400"},
    {0x243, "i80321-600"},    {0x290, "PXA250B/PXA26x"},
    {0x292, "PXA210B"},       {0x2c2, "i80321-400-B0"},
    {0x2c3, "i80321-600-B0"}, {0x2d0, "PXA250C/PXA255/PXA26x"},
    {0x2d2, "PXA210C"},       {0x411, "PXA27x"},
    {0x41c, "IPX425-533"},    {0x41d, "IPX425-400"},
    {0x41f, "IPX425-266"},    {0x682, "PXA32x"},
    {0x683, "PXA930/PXA935"}, {0x688, "PXA30x"},
    {0x689, "PXA31x"},        {0xb11, "SA1110"},
    {0xc12, "IPX1200"},       {-1, "unknown"},
};

static const struct id_part fujitsu_part[] = {
    {0x001, "A64FX"},
    {0x003, "MONAKA"},
    {-1, "unknown"},
};

static const struct id_part hisi_part[] = {
    {0xd01, "TaiShan-v110"}, /* used in Kunpeng-920 SoC */
    {0xd02, "TaiShan-v120"}, /* used in Kirin 990A and 9000S SoCs */
    {0xd40, "Cortex-A76"},   /* HiSilicon uses this ID though advertises A76 */
    {0xd41, "Cortex-A77"},   /* HiSilicon uses this ID though advertises A77 */
    {-1, "unknown"},
};

static const struct id_part ampere_part[] = {
    {0xac3, "Ampere-1"},
    {0xac4, "Ampere-1a"},
    {-1, "unknown"},
};

static const struct id_part ft_part[] = {
    {0x303, "FTC310"}, {0x660, "FTC660"}, {0x661, "FTC661"}, {0x662, "FTC662"},
    {0x663, "FTC663"}, {0x664, "FTC664"}, {0x862, "FTC862"}, {-1, "unknown"},
};

static const struct id_part ms_part[] = {
    {0xd49, "Azure-Cobalt-100"},
    {-1, "unknown"},
};

static const struct id_part unknown_part[] = {
    {-1, "unknown"},
};

struct hw_impl
{
    const int id;
    const struct id_part *parts;
    const char *name;
};

static const struct hw_impl hw_implementer[] = {
    {0x41, arm_part, "ARM"},          {0x42, brcm_part, "Broadcom"},
    {0x43, cavium_part, "Cavium"},    {0x44, dec_part, "DEC"},
    {0x46, fujitsu_part, "FUJITSU"},  {0x48, hisi_part, "HiSilicon"},
    {0x49, unknown_part, "Infineon"}, {0x4d, unknown_part, "Motorola/Freescale"},
    {0x4e, nvidia_part, "NVIDIA"},    {0x50, apm_part, "APM"},
    {0x51, qcom_part, "Qualcomm"},    {0x53, samsung_part, "Samsung"},
    {0x56, marvell_part, "Marvell"},  {0x61, apple_part, "Apple"},
    {0x66, faraday_part, "Faraday"},  {0x69, intel_part, "Intel"},
    {0x6d, ms_part, "Microsoft"},     {0x70, ft_part, "Phytium"},
    {0xc0, ampere_part, "Ampere"},    {-1, unknown_part, "unknown"},
};
static std::string aarch64_tocpuname(uint64_t d)
{
    auto impl = d >> 24 & 0xff;
    auto part = d >> 4 & 0xfff;

    const char *manu;
    const char *name;
    for (int imp = 0; imp < 20; imp++)
    {
        if (impl == hw_implementer[imp].id)
        {
            manu = hw_implementer[imp].name;
            auto pt = hw_implementer[imp].parts;
            while (pt->id != -1)
            {
                if (pt->id == part)
                {
                    name = pt->name;
                    break;
                }
                pt++;
            }
        }
    }
    if (!manu)
    {
        manu = "Unknown";
    }
    else if (!name)
    {
        name = "Unknown";
    }
    return fmt::format("{} {}", manu, name);
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
static std::string fetchFromDevFs()
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
std::string fetchCpuName()
{
    fetchFromDevFs();
    struct utsname n;
    uname(&n);
#if defined(__x86_64__) || defined(__x86__)
    char model[64] = {0};
    int32_t *d = (int32_t *)model;
    cpuinfo_x86(0x80000002, &d[0], &d[1], &d[2], &d[3]);
    cpuinfo_x86(0x80000003, &d[4], &d[5], &d[6], &d[7]);
    cpuinfo_x86(0x80000004, &d[8], &d[9], &d[10], &d[11]);
    model[48] = '\0';
    return std::string(model);
#elif defined(__aarch64__)
    return aarch64_tocpuname(cpuinfo_aarch64());
#elif defined(__arm__)
    return aarch64_tocpuname(cpuinfo_arm());
#else
    auto st = fetchFromDevFs();
    return st == "" ? fmt::format("unknown {} cpu", n.machine) : st;
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
} // namespace openminecraft::vm::os