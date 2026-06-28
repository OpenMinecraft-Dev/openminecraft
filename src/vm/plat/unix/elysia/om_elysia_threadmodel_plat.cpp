#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include <pthread.h>

#ifdef OM_PLATFORM_ANDROID
int pthread_getname_np(pthread_t, char *, int);
#endif

namespace openminecraft::vm::elysia
{
std::string OMElysiaThread::getName()
{
#ifdef OM_PLATFORM_APPLE
    return threadName;
#else
    char name[1024];
    pthread_getname_np((pthread_t)nativeHandle, name, 1024);
    return name;
#endif
}
void OMElysiaThread::setName(std::string n)
{
    threadName = n;
#ifdef OM_PLATFORM_APPLE
    thread_set_thread_name((thread_t)nativeHandle, n.c_str());
#else
    pthread_setname_np((pthread_t)nativeHandle, n.c_str());
#endif
}
void OMElysiaThread::initInternals()
{
#ifdef OM_PLATFORM_APPLE
    nativeHandle = (void *)thread_self();
#else
    nativeHandle = (void *)pthread_self();
#endif
}
} // namespace openminecraft::vm::elysia
