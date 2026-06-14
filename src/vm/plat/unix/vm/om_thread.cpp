#include "openminecraft/vm/os/om_thread.hpp"
#include <cstring>
#include <pthread.h>

#ifdef OM_PLATFORM_ANDROID
#include <sys/prctl.h>
#endif

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
    char name[64];
    std::memset(name, 0x00, 64);
#if defined(OM_PLATFORM_ANDROID)
    prctl(PR_GET_NAME, name);
#else
    pthread_getname_np(pthread_self(), name, 64);
#endif
    return std::string(name);
}
} // namespace openminecraft::vm::os
