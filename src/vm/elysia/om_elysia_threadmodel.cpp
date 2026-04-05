#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"

namespace openminecraft::vm::elysia
{
thread_local OMElysiaThread threadContext;
std::unordered_map<std::thread::id, OMElysiaThread *> threadMap;
} // namespace openminecraft::vm::elysia
