#ifndef OM_CPUNAME_HPP
#define OM_CPUNAME_HPP

#include <cstdint>
#include <string>
namespace openminecraft::vm::os
{
std::string fetchCpuName();
std::string fetchUsername();
std::string fetchLoginUser();
std::string fetchSystemName();
std::string fetchSystemVersion();

uint64_t fetchMemoryTotal();
uint64_t fetchMemoryAvailable();
} // namespace openminecraft::vm::os

#endif