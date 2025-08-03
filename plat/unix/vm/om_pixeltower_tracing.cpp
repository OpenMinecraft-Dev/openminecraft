#include "openminecraft/vm/pixeltower/v1/om_pixeltower_tracing.hpp"
#include "boost/stacktrace/stacktrace.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "sys/signal.h"
#include <csignal>
#include <cstdint>
#include <sys/ucontext.h>
#include <unistd.h>
#include <vector>

namespace openminecraft::vm::pixeltower::v1::tracing
{
std::map<std::string, void *> registers;
static void crash_handler(int sig, siginfo_t *info, void *context)
{
    ucontext_t *const uc = (ucontext_t *)context;
    log::OMLogger l("Crash Handler");
    l.info("signo: {}, {}", info->si_signo, getpid());

    void *pc = nullptr;

#ifdef __x86_64__
#if defined(__APPLE__)
    pc = (void *)uc->uc_mcontext->__ss.__rip;
#define storeReg(n, idx) registers[n] = (void *)uc->uc_mcontext->__ss.__##idx;
    storeReg("r8", r8);
    storeReg("r9", r9);
    storeReg("r10", r10);
    storeReg("r11", r11);
    storeReg("r12", r12);
    storeReg("r13", r13);
    storeReg("r14", r14);
    storeReg("r15", r15);
    storeReg("rdi", rdi);
    storeReg("rsi", rsi);
    storeReg("rbp", rbp);
    storeReg("rbx", rbx);
    storeReg("rdx", rdx);
    storeReg("rax", rax);
    storeReg("rcx", rcx);
    storeReg("rsp", rsp);

#elif defined(OM_PLATFORM_BSD)
    pc = (void *)uc->uc_mcontext.mc_rip;
#define storeReg(n, idx) registers[n] = (void *)uc->uc_mcontext.##idx;
    storeReg("r8", r8);
    storeReg("r9", r9);
    storeReg("r10", r10);
    storeReg("r11", r11);
    storeReg("r12", r12);
    storeReg("r13", r13);
    storeReg("r14", r14);
    storeReg("r15", r15);
    storeReg("rdi", rdi);
    storeReg("rsi", rsi);
    storeReg("rbp", rbp);
    storeReg("rbx", rbx);
    storeReg("rdx", rdx);
    storeReg("rax", rax);
    storeReg("rcx", rcx);
    storeReg("rsp", rsp);
#else
    pc = (void *)uc->uc_mcontext.gregs[REG_RIP];
#define storeReg(n, idx) registers[n] = (void *)uc->uc_mcontext.gregs[REG_##idx];
    storeReg("r8", R8);
    storeReg("r9", R9);
    storeReg("r10", R10);
    storeReg("r11", R11);
    storeReg("r12", R12);
    storeReg("r13", R13);
    storeReg("r14", R14);
    storeReg("r15", R15);
    storeReg("rdi", RDI);
    storeReg("rsi", RSI);
    storeReg("rbp", RBP);
    storeReg("rbx", RBX);
    storeReg("rdx", RDX);
    storeReg("rax", RAX);
    storeReg("rcx", RCX);
    storeReg("rsp", RSP);

    storeReg("efl", EFL);
    storeReg("csgsfs", CSGSFS);
    storeReg("err", ERR);
    storeReg("trapno", TRAPNO);

    registers["mxcsr"] = (void *)(size_t)uc->uc_mcontext.fpregs->mxcsr;
    for (int xmmid = 0; xmmid < 16; xmmid++)
    {
        void **data = (void **)&uc->uc_mcontext.fpregs->_xmm[xmmid];
        registers[fmt::format("xmmh{}", xmmid)] = data[1];
        registers[fmt::format("xmml{}", xmmid)] = data[0];
    }
#endif
#endif

#ifdef __aarch64__
#if defined(__APPLE__)
    pc = (void *)uc->uc_mcontext->__ss.pc;
#endif
#endif

    registers["pc"] = pc;

    for (auto &reg : registers)
    {
        l.info("Reg {}: {}", reg.first, reg.second);
    }

    std::vector<OMTracingFrame> frames;
    boost::stacktrace::stacktrace s;
    for (auto f : s)
    {
        frames.push_back(OMTracingFrame{(void *)f.address(), f.name()});
    }

    for (auto &ff : frames)
    {
        l.info("{} @ {}", ff.location, ff.name);
    }

    raise(SIGKILL);
    abort();
}

typedef void (*sighandler)(int, siginfo_t *, void *);
int install_sigaction_signal_handler(struct sigaction *sigAct, struct sigaction *oldSigAct, int sig, sighandler handler)
{
    sigfillset(&sigAct->sa_mask);
    // remove_error_signals_from_set(&sigAct->sa_mask);
    sigAct->sa_sigaction = (sighandler)handler;
    sigAct->sa_flags = SA_SIGINFO | SA_RESTART;
#if defined(__APPLE__)
    if (sig == SIGSEGV)
    {
        sigAct->sa_flags |= SA_ONSTACK;
    }
#endif
    return sigaction(sig, sigAct, oldSigAct);
}

void installHandler()
{
    static const int signals_to_handle[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGTRAP, 0};
    for (int i = 0; signals_to_handle[i] != 0; i++)
    {
        struct sigaction sigAct, oldSigAct;
        install_sigaction_signal_handler(&sigAct, &oldSigAct, signals_to_handle[i], crash_handler);
    }
}
} // namespace openminecraft::vm::pixeltower::v1::tracing
