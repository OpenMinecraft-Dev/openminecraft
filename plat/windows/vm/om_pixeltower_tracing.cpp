#include "openminecraft/vm/pixeltower/v1/om_pixeltower_tracing.hpp"
#include "Windows.h"
#include "boost/stacktrace/detail/frame_decl.hpp"
#include "dbgHelp.h"
#include "openminecraft/log/om_log_common.hpp"
#include <iostream>
#include <minwindef.h>
#include <processthreadsapi.h>
#include <vector>
#include <winnt.h>

namespace openminecraft::vm::pixeltower::v1::tracing
{
static long WINAPI handler(_EXCEPTION_POINTERS *pt)
{
    SymInitialize(GetCurrentProcess(), nullptr, true);
    auto proc = GetCurrentProcess();
    auto thr = GetCurrentThread();
    auto stackFrame = STACKFRAME64{0};
    auto context = pt->ContextRecord;

#if defined(_M_IX86)
    stackFrame.AddrPC.Offset = context->Eip;
    stackFrame.AddrStack.Offset = context->Esp;
    stackFrame.AddrFrame.Offset = context->Ebp;
    auto machineType = IMAGE_FILE_MACHINE_I386;
#elif defined(_M_X64)
    stackFrame.AddrPC.Offset = context->Rip;
    stackFrame.AddrStack.Offset = context->Rsp;
    stackFrame.AddrFrame.Offset = context->Rbp;
    auto machineType = IMAGE_FILE_MACHINE_AMD64;
#elif defined(_M_ARM64)
    stackFrame.AddrPC.Offset = context->Pc;
    stackFrame.AddrStack.Offset = context->Sp;
    stackFrame.AddrFrame.Offset = context->Fp;
    auto machineType = IMAGE_FILE_MACHINE_ARM64;
#endif
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Mode = AddrModeFlat;

    std::vector<OMTracingFrame> frames;
    while (StackWalk64(machineType, proc, thr, &stackFrame, context, nullptr, SymFunctionTableAccess64,
                       SymGetModuleBase64, nullptr))
    {
        BYTE symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {0};
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbolBuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 di = 0;
        auto res = SymFromAddr(proc, stackFrame.AddrPC.Offset, &di, symbol);

        if (res)
        {
            frames.push_back(OMTracingFrame{(void *)stackFrame.AddrPC.Offset, std::string(symbol->Name)});
        }
        else
        {
            frames.push_back(OMTracingFrame{(void *)stackFrame.AddrPC.Offset, ""});
        }
    }

    log::OMLogger l("Crash handler");
    for (auto &ff : frames)
    {
        l.info("{} @ {}", ff.location, ff.name);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}
void installHandler()
{
    SetUnhandledExceptionFilter(handler);
}
} // namespace openminecraft::vm::pixeltower::v1::tracing