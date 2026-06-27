#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include <pthread.h>

#ifdef OM_PLATFORM_ANDROID
int pthread_getname_np(pthread_t, char *, int);
#endif

namespace openminecraft::vm::elysia
{
std::string OMElysiaThread::getName()
{
    char name[1024];
    pthread_getname_np((pthread_t)nativeHandle, name, 1024);
    return name;
}
void OMElysiaThread::setName(std::string n)
{
    pthread_setname_np((pthread_t)nativeHandle, n.c_str());
}
void OMElysiaThread::initInternals()
{
    nativeHandle = (void *)pthread_self();
}
} // namespace openminecraft::vm::elysia
