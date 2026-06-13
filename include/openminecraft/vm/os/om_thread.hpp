#ifndef OM_THREAD_HPP
#define OM_THREAD_HPP

#include <cstdint>
#include <string>

namespace openminecraft::vm::os
{
uintptr_t fetchCurrentThread();
void threadSetName(std::string);
std::string threadGetName(uintptr_t);
} // namespace openminecraft::vm::os

#endif
