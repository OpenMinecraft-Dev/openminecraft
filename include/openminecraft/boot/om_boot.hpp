#ifndef OM_BOOT_HPP
#define OM_BOOT_HPP

#include <string>
#include <vector>

namespace openminecraft::boot
{
void onCrash(int sig);
int boot(std::vector<std::string> args);
} // namespace openminecraft::boot

#endif