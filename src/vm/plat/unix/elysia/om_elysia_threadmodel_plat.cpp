#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include <pthread.h>

#ifdef OM_PLATFORM_ANDROID
int pthread_getname_np(pthread_t, char *, int);
#endif

namespace openminecraft::vm::elysia
{
std::string OMElysiaThread::getName()
{
    return threadName;
}
void OMElysiaThread::setName(std::string n)
{
    threadName = n;
#ifdef OM_PLATFORM_APPLE
    thread_set_name(pthread_mach_thread_np((pthread_t)nativeHandle), n.c_str());
#else
    pthread_setname_np((pthread_t)nativeHandle, n.c_str());
#endif
}
void OMElysiaThread::initInternals()
{
    nativeHandle = (void *)pthread_self();
}
} // namespace openminecraft::vm::elysia
