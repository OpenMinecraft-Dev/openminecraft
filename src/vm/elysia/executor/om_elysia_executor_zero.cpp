#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <cstdint>

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
    if (!threadContext.threadInited)
    {
        threadContext.stackEnd = mem::allocator::tracedMallocElysia(1024 * 1024);
        threadContext.stackStart = reinterpret_cast<uint8_t *>(threadContext.stackEnd) + 1024 * 1024;
        threadContext.zero.stackPointer = threadContext.stackStart;
        threadContext.threadInited = true;

        threadContext.cleaner = [&]() { mem::allocator::tracedFreeElysia(threadContext.stackEnd); };

        logger.info("Thread Init!");
    }

    threadContext.zero.pc = m->code;
    threadContext.zero.stackPointer = reinterpret_cast<void *>(
        reinterpret_cast<uintptr_t>(threadContext.zero.stackPointer) - sizeof(OMElysiaJavaFrame));
    auto frame = reinterpret_cast<OMElysiaJavaFrame *>(threadContext.zero.stackPointer);
    frame->method = m;
    frame->caller = threadContext.zero.frame;
    threadContext.zero.frame = frame;

    std::cout << (void *)threadContext.zero.frame << std::endl;
    throw 0;
}
} // namespace openminecraft::vm::elysia::executor
