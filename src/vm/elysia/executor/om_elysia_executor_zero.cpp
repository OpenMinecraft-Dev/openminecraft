#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <cstdint>
#include <thread>

namespace openminecraft::vm::elysia::executor
{
OMElysiaExecutorZero::OMElysiaExecutorZero(OMElysiaVirtualWorld *vw) : world(vw), logger("OMElysiaExecutorZero", this)
{
}
OMElysiaExecutorZero::~OMElysiaExecutorZero()
{
}

void OMElysiaExecutorZero::execute(OMElysiaMethod *m)
{
    auto tc = thisThread.metadata;
    if (!tc->threadInited)
    {
        tc->stackEnd = mem::allocator::tracedMallocElysia(1024 * 1024);
        tc->stackStart = reinterpret_cast<uint8_t *>(tc->stackEnd) + 1024 * 1024;
        tc->zero.stackPointer = tc->stackStart;
        tc->threadInited = true;

        tc->cleaner = [&]() { mem::allocator::tracedFreeElysia(tc->stackEnd); };
    }

    tc->zero.pc = m->code;
    tc->zero.stackPointer =
        reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(tc->zero.stackPointer) - sizeof(OMElysiaJavaFrame));
    auto frame = reinterpret_cast<OMElysiaJavaFrame *>(tc->zero.stackPointer);
    frame->method = m;
    frame->caller = tc->zero.frame;
    tc->zero.frame = frame;

    while (true)
    {
    }
}
} // namespace openminecraft::vm::elysia::executor
