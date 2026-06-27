#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include <pthread.h>

namespace openminecraft::vm::elysia
{
std::string OMElysiaThread::getName()
{
    return "";
}
void OMElysiaThread::setName(std::string)
{
}
void OMElysiaThread::initInternals()
{
    nativeHandle = (void *)pthread_self();
}
} // namespace openminecraft::vm::elysia
