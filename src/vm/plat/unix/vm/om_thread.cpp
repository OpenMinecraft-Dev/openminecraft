#include "openminecraft/vm/os/om_thread.hpp"
#include <cstdint>
#include <pthread.h>

namespace openminecraft::vm::os
{
void threadSetName(std::string name)
{
#if defined(OM_PLATFORM_MACOS) || defined(OM_PLATFORM_IOS)
    pthread_setname_np(name.c_str());
#else
    pthread_setname_np(pthread_self(), name.c_str());
#endif
}
std::string threadGetName()
{
    return "";
}
} // namespace openminecraft::vm::os
