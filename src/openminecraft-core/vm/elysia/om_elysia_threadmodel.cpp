#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"

namespace openminecraft::vm::elysia
{
thread_local OMElysiaThreadMetadata thisThread;
std::map<std::thread::id, OMElysiaThread *> threadMap;
std::mutex mapMutex;
} // namespace openminecraft::vm::elysia
