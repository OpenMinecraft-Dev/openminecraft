#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include <pthread.h>

namespace openminecraft::vm::elysia
{
auto OMElysiaThread::getName() -> std::string
{
    return threadName;
}
void OMElysiaThread::setName(std::string n)
{
    threadName = n;
#ifndef OM_PLATFORM_APPLE
    pthread_setname_np(reinterpret_cast<pthread_t>(nativeHandle), n.c_str());
#endif
}
void OMElysiaThread::initInternals()
{
    nativeHandle = reinterpret_cast<void *>(pthread_self());
}
} // namespace openminecraft::vm::elysia
