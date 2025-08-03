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
std::map<std::string, void *> registers;
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

    registers["pc"] = (void *)context->Eip;
    registers["eax"] = (void *)context->Eax;
    registers["ebx"] = (void *)context->Ebx;
    registers["ecx"] = (void *)context->Ecx;
    registers["edx"] = (void *)context->Edx;
    registers["esp"] = (void *)context->Esp;
    registers["ebp"] = (void *)context->Ebp;
    registers["esi"] = (void *)context->Esi;
    registers["edi"] = (void *)context->Edi;
    registers["efl"] = (void *)context->EFlags;
    auto machineType = IMAGE_FILE_MACHINE_I386;
#elif defined(_M_X64)
    stackFrame.AddrPC.Offset = context->Rip;
    stackFrame.AddrStack.Offset = context->Rsp;
    stackFrame.AddrFrame.Offset = context->Rbp;

    registers["pc"] = (void *)context->Rip;
    registers["rax"] = (void *)context->Rax;
    registers["rbx"] = (void *)context->Rbx;
    registers["rcx"] = (void *)context->Rcx;
    registers["rdx"] = (void *)context->Rdx;
    registers["rsp"] = (void *)context->Rsp;
    registers["rbp"] = (void *)context->Rbp;
    registers["rsi"] = (void *)context->Rsi;
    registers["rdi"] = (void *)context->Rdi;

    registers["r8"] = (void *)context->R8;
    registers["r9"] = (void *)context->R9;
    registers["r10"] = (void *)context->R10;
    registers["r11"] = (void *)context->R11;
    registers["r12"] = (void *)context->R12;
    registers["r13"] = (void *)context->R13;
    registers["r14"] = (void *)context->R14;
    registers["r15"] = (void *)context->R15;

#define regxmm(tgt, n, hn, ln)          \
    void ** n = (void **)&context->tgt; \
    registers[hn] = n[1];               \
    registers[ln] = n[0];

    regxmm(Xmm0, xmm0, "xmmh0", "xmml0");
    regxmm(Xmm1, xmm1, "xmmh1", "xmml1");
    regxmm(Xmm2, xmm2, "xmmh2", "xmml2");
    regxmm(Xmm3, xmm3, "xmmh3", "xmml3");
    regxmm(Xmm4, xmm4, "xmmh4", "xmml4");
    regxmm(Xmm5, xmm5, "xmmh5", "xmml5");
    regxmm(Xmm6, xmm6, "xmmh6", "xmml6");
    regxmm(Xmm7, xmm7, "xmmh7", "xmml7");
    regxmm(Xmm8, xmm8, "xmmh8", "xmml8");
    regxmm(Xmm9, xmm9, "xmmh9", "xmml9");
    regxmm(Xmm10, xmm10, "xmmh10", "xmml10");
    regxmm(Xmm11, xmm11, "xmmh11", "xmml11");
    regxmm(Xmm12, xmm12, "xmmh12", "xmml12");
    regxmm(Xmm13, xmm13, "xmmh13", "xmml13");
    regxmm(Xmm14, xmm14, "xmmh14", "xmml14");
    regxmm(Xmm15, xmm15, "xmmh15", "xmml15");

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
    for (auto &regs : registers)
    {
        l.info("Reg {}: {}", regs.first, regs.second);
    }

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