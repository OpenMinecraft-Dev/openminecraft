#ifndef OM_THREAD_HPP
#define OM_THREAD_HPP

#include <cstdint>
#include <string>

namespace openminecraft::vm::os
{
void threadSetName(std::string);
std::string threadGetName();
} // namespace openminecraft::vm::os

#endif
