#ifndef OM_HARDWARE_HPP
#define OM_HARDWARE_HPP

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
uint64_t fetchAvailableProcessors();
} // namespace openminecraft::vm::os

#endif
