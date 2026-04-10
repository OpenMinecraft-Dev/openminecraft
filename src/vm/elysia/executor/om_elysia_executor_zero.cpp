#include "openminecraft/vm/elysia/executor/om_elysia_executor_zero.hpp"
#include "ffi.h"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "openminecraft/vm/elysia/om_elysia_method.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <cstdint>
#include <thread>

namespace openminecraft::vm::elysia::executor
{
static int add(int a, int b)
{
    return a + b;
}

OMElysiaExecutorZero::OMElysiaExecutorZero(OMElysiaVirtualWorld *vw) : world(vw), logger("OMElysiaExecutorZero", this)
{
    auto functionPtr = (void (*)())&add;
    int argCount = 2;

    ffi_type **ffiArgTypes = (ffi_type **)malloc(sizeof(ffi_type *) * argCount);
    ffiArgTypes[0] = &ffi_type_sint;
    ffiArgTypes[1] = &ffi_type_sint;

    void **ffiArgs = (void **)malloc(sizeof(void *) * argCount);
    int a1 = 5;
    int a2 = 3;
    ffiArgs[0] = &a1;
    ffiArgs[1] = &a2;

    ffi_cif cif;
    ffi_type *returnFfiType = &ffi_type_sint;
    ffi_status ffiPrepStatus = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, (unsigned int)argCount, returnFfiType, ffiArgTypes);

    if (ffiPrepStatus == FFI_OK)
    {
        void *returnPtr = NULL;
        if (returnFfiType->size)
        {
            returnPtr = malloc(returnFfiType->size);
        }
        ffi_call(&cif, functionPtr, returnPtr, ffiArgs);

        int returnValue = *(int *)returnPtr;
        logger.info("ret: {}", returnValue);
    }
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
    for (int i = 0; i < m->localLength; i++) {
        zeroStackPush<jint>(0);
    }
    frame->method = m;
    frame->caller = (OMElysiaJavaFrame *)0x33550336;
    tc->zero.frame = frame;

    while (true)
    {
        switch (*reinterpret_cast<uint8_t *>(tc->zero.pc))
        {
        case op_nop:
            ++tc->zero.pc;
            break;
#define op_iconst(n)                                                                                                   \
    case op_iconst_i(n):                                                                                               \
        zeroStackPush<jint>(n);                                                                                        \
        ++tc->zero.pc;                                                                                                 \
        break;

            op_iconst(-1);
	    op_iconst(0);
	    op_iconst(1);
	    op_iconst(2);
	    op_iconst(3);
	    op_iconst(4);
	    op_iconst(5);
        default:
            while (true)
            {
                continue;
            }
        }
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
    tc->zero.stackPointer = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(tc->zero.stackPointer) + len);
    return result;
}
} // namespace openminecraft::vm::elysia::executor
