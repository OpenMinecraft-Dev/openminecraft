#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
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
    auto frame = reinterpret_cast<OMElysiaJavaFrame *>(zeroStackAlloc(sizeof(OMElysiaJavaFrame)));
    frame->method = m;
    frame->caller = (OMElysiaJavaFrame *)0x33550336;
    tc->zero.frame = frame;

    while (true)
    {
    }
}

void *zeroStackAlloc(uint64_t len)
{
    auto tc = thisThread.metadata;
    tc->zero.stackPointer = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(tc->zero.stackPointer) - len);
    return tc->zero.stackPointer;
}

void *zeroStackPop(uint64_t len)
{
    auto tc = thisThread.metadata;
    auto result = tc->zero.stackPointer;
    tc->zero.stackPointer = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(tc->zero
.stackPointer) + len);
    return result;
}
} // namespace openminecraft::vm::elysia::executor
