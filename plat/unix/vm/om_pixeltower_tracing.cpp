#include "openminecraft/vm/pixeltower/v1/om_pixeltower_tracing.hpp"
#include "boost/stacktrace/stacktrace.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "sys/signal.h"
#include <csignal>
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

#ifdef __i386__
#if defined(OM_PLATFORM_BSD) || defined(__APPLE__)
#ifdef __FreeBSD__
#define context_pc uc_mcontext.mc_eip
#define context_sp uc_mcontext.mc_esp
#define context_fp uc_mcontext.mc_ebp
#define context_eip uc_mcontext.mc_eip
#define context_esp uc_mcontext.mc_esp
#define context_eax uc_mcontext.mc_eax
#define context_ebx uc_mcontext.mc_ebx
#define context_ecx uc_mcontext.mc_ecx
#define context_edx uc_mcontext.mc_edx
#define context_ebp uc_mcontext.mc_ebp
#define context_esi uc_mcontext.mc_esi
#define context_edi uc_mcontext.mc_edi
#define context_eflags uc_mcontext.mc_eflags
#define context_trapno uc_mcontext.mc_trapno
#endif

#ifdef __APPLE__
#if __DARWIN_UNIX03 && (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
// 10.5 UNIX03 member name prefixes
#define DU3_PREFIX(s, m) __##s.__##m
#else
#define DU3_PREFIX(s, m) s##.##m
#endif

#define context_pc context_eip
#define context_sp context_esp
#define context_fp context_ebp
#define context_eip uc_mcontext->DU3_PREFIX(ss, eip)
#define context_esp uc_mcontext->DU3_PREFIX(ss, esp)
#define context_eax uc_mcontext->DU3_PREFIX(ss, eax)
#define context_ebx uc_mcontext->DU3_PREFIX(ss, ebx)
#define context_ecx uc_mcontext->DU3_PREFIX(ss, ecx)
#define context_edx uc_mcontext->DU3_PREFIX(ss, edx)
#define context_ebp uc_mcontext->DU3_PREFIX(ss, ebp)
#define context_esi uc_mcontext->DU3_PREFIX(ss, esi)
#define context_edi uc_mcontext->DU3_PREFIX(ss, edi)
#define context_eflags uc_mcontext->DU3_PREFIX(ss, eflags)
#define context_trapno uc_mcontext->DU3_PREFIX(es, trapno)
#endif

#ifdef __OpenBSD__
#define context_pc sc_eip
#define context_sp sc_esp
#define context_fp sc_ebp
#define context_eip sc_eip
#define context_esp sc_esp
#define context_eax sc_eax
#define context_ebx sc_ebx
#define context_ecx sc_ecx
#define context_edx sc_edx
#define context_ebp sc_ebp
#define context_esi sc_esi
#define context_edi sc_edi
#define context_eflags sc_eflags
#define context_trapno sc_trapno
#endif

#ifdef __NetBSD__
#define context_pc uc_mcontext.__gregs[_REG_EIP]
#define context_sp uc_mcontext.__gregs[_REG_UESP]
#define context_fp uc_mcontext.__gregs[_REG_EBP]
#define context_eip uc_mcontext.__gregs[_REG_EIP]
#define context_esp uc_mcontext.__gregs[_REG_UESP]
#define context_eax uc_mcontext.__gregs[_REG_EAX]
#define context_ebx uc_mcontext.__gregs[_REG_EBX]
#define context_ecx uc_mcontext.__gregs[_REG_ECX]
#define context_edx uc_mcontext.__gregs[_REG_EDX]
#define context_ebp uc_mcontext.__gregs[_REG_EBP]
#define context_esi uc_mcontext.__gregs[_REG_ESI]
#define context_edi uc_mcontext.__gregs[_REG_EDI]
#define context_eflags uc_mcontext.__gregs[_REG_EFL]
#define context_trapno uc_mcontext.__gregs[_REG_TRAPNO]
#endif

    pc = (void *)uc->context_pc;
#define storeReg(n, idx) registers[n] = (void *)uc->##idx;
    storeReg("eax", context_eax);
    storeReg("ebx", context_ebx);
    storeReg("ecx", context_ecx);
    storeReg("edx", context_edx);
    storeReg("esp", context_esp);
    storeReg("ebp", context_ebp);
    storeReg("esi", context_esi);
    storeReg("edi", context_edi);
    storeReg("efl", context_eflags);
    storeReg("trapno", context_trapno);
#else
    pc = (void *)uc->uc_mcontext.gregs[REG_EIP];
#define storeReg(n, idx) registers[n] = (void *)uc->uc_mcontext.gregs[REG_##idx];
    storeReg("eax", EAX);
    storeReg("ebx", EBX);
    storeReg("ecx", ECX);
    storeReg("edx", EDX);
    storeReg("esp", ESP);
    storeReg("ebp", EBP);
    storeReg("esi", ESI);
    storeReg("edi", EDI);
    storeReg("efl", EFL);
#endif
#endif

#ifdef __x86_64__
#if defined(OM_PLATFORM_BSD) || defined(__APPLE__)
#ifdef __FreeBSD__
#define context_trapno uc_mcontext.mc_trapno
#define context_pc uc_mcontext.mc_rip
#define context_sp uc_mcontext.mc_rsp
#define context_fp uc_mcontext.mc_rbp
#define context_rip uc_mcontext.mc_rip
#define context_rsp uc_mcontext.mc_rsp
#define context_rbp uc_mcontext.mc_rbp
#define context_rax uc_mcontext.mc_rax
#define context_rbx uc_mcontext.mc_rbx
#define context_rcx uc_mcontext.mc_rcx
#define context_rdx uc_mcontext.mc_rdx
#define context_rsi uc_mcontext.mc_rsi
#define context_rdi uc_mcontext.mc_rdi
#define context_r8 uc_mcontext.mc_r8
#define context_r9 uc_mcontext.mc_r9
#define context_r10 uc_mcontext.mc_r10
#define context_r11 uc_mcontext.mc_r11
#define context_r12 uc_mcontext.mc_r12
#define context_r13 uc_mcontext.mc_r13
#define context_r14 uc_mcontext.mc_r14
#define context_r15 uc_mcontext.mc_r15
#define context_flags uc_mcontext.mc_flags
#define context_err uc_mcontext.mc_err
#endif

#ifdef __APPLE__
#if __DARWIN_UNIX03 && (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
// 10.5 UNIX03 member name prefixes
#define DU3_PREFIX(s, m) __##s.__##m
#else
#define DU3_PREFIX(s, m) ##s.##m
#endif

#define context_pc context_rip
#define context_sp context_rsp
#define context_fp context_rbp
#define context_rip uc_mcontext->DU3_PREFIX(ss, rip)
#define context_rsp uc_mcontext->DU3_PREFIX(ss, rsp)
#define context_rax uc_mcontext->DU3_PREFIX(ss, rax)
#define context_rbx uc_mcontext->DU3_PREFIX(ss, rbx)
#define context_rcx uc_mcontext->DU3_PREFIX(ss, rcx)
#define context_rdx uc_mcontext->DU3_PREFIX(ss, rdx)
#define context_rbp uc_mcontext->DU3_PREFIX(ss, rbp)
#define context_rsi uc_mcontext->DU3_PREFIX(ss, rsi)
#define context_rdi uc_mcontext->DU3_PREFIX(ss, rdi)
#define context_r8 uc_mcontext->DU3_PREFIX(ss, r8)
#define context_r9 uc_mcontext->DU3_PREFIX(ss, r9)
#define context_r10 uc_mcontext->DU3_PREFIX(ss, r10)
#define context_r11 uc_mcontext->DU3_PREFIX(ss, r11)
#define context_r12 uc_mcontext->DU3_PREFIX(ss, r12)
#define context_r13 uc_mcontext->DU3_PREFIX(ss, r13)
#define context_r14 uc_mcontext->DU3_PREFIX(ss, r14)
#define context_r15 uc_mcontext->DU3_PREFIX(ss, r15)
#define context_flags uc_mcontext->DU3_PREFIX(ss, rflags)
#define context_trapno uc_mcontext->DU3_PREFIX(es, trapno)
#define context_err uc_mcontext->DU3_PREFIX(es, err)
#endif

#ifdef __OpenBSD__
#define context_trapno sc_trapno
#define context_pc sc_rip
#define context_sp sc_rsp
#define context_fp sc_rbp
#define context_rip sc_rip
#define context_rsp sc_rsp
#define context_rbp sc_rbp
#define context_rax sc_rax
#define context_rbx sc_rbx
#define context_rcx sc_rcx
#define context_rdx sc_rdx
#define context_rsi sc_rsi
#define context_rdi sc_rdi
#define context_r8 sc_r8
#define context_r9 sc_r9
#define context_r10 sc_r10
#define context_r11 sc_r11
#define context_r12 sc_r12
#define context_r13 sc_r13
#define context_r14 sc_r14
#define context_r15 sc_r15
#define context_flags sc_rflags
#define context_err sc_err
#endif

#ifdef __NetBSD__
#define context_trapno uc_mcontext.__gregs[_REG_TRAPNO]
#define __register_t __greg_t
#define context_pc uc_mcontext.__gregs[_REG_RIP]
#define context_sp uc_mcontext.__gregs[_REG_URSP]
#define context_fp uc_mcontext.__gregs[_REG_RBP]
#define context_rip uc_mcontext.__gregs[_REG_RIP]
#define context_rsp uc_mcontext.__gregs[_REG_URSP]
#define context_rax uc_mcontext.__gregs[_REG_RAX]
#define context_rbx uc_mcontext.__gregs[_REG_RBX]
#define context_rcx uc_mcontext.__gregs[_REG_RCX]
#define context_rdx uc_mcontext.__gregs[_REG_RDX]
#define context_rbp uc_mcontext.__gregs[_REG_RBP]
#define context_rsi uc_mcontext.__gregs[_REG_RSI]
#define context_rdi uc_mcontext.__gregs[_REG_RDI]
#define context_r8 uc_mcontext.__gregs[_REG_R8]
#define context_r9 uc_mcontext.__gregs[_REG_R9]
#define context_r10 uc_mcontext.__gregs[_REG_R10]
#define context_r11 uc_mcontext.__gregs[_REG_R11]
#define context_r12 uc_mcontext.__gregs[_REG_R12]
#define context_r13 uc_mcontext.__gregs[_REG_R13]
#define context_r14 uc_mcontext.__gregs[_REG_R14]
#define context_r15 uc_mcontext.__gregs[_REG_R15]
#define context_flags uc_mcontext.__gregs[_REG_RFL]
#define context_err uc_mcontext.__gregs[_REG_ERR]
#endif

    pc = (void *)uc->context_pc;
#define storeReg(n, idx) registers[n] = (void *)uc->##idx;
    storeReg("r8", context_r8);
    storeReg("r9", context_r9);
    storeReg("r10", context_r10);
    storeReg("r11", context_r11);
    storeReg("r12", context_r12);
    storeReg("r13", context_r13);
    storeReg("r14", context_r14);
    storeReg("r15", context_r15);
    storeReg("rdi", context_rdi);
    storeReg("rsi", context_rsi);
    storeReg("rbp", context_rbp);
    storeReg("rbx", context_rbx);
    storeReg("rdx", context_rdx);
    storeReg("rax", context_rax);
    storeReg("rcx", context_rcx);
    storeReg("rsp", context_rsp);
    storeReg("trapno", context_trapno);
    storeReg("rfl", context_flags);
    storeReg("err", context_err);
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
#if defined(OM_PLATFORM_BSD) || defined(__APPLE__)

#ifdef __APPLE__
// see darwin-xnu/osfmk/mach/arm/_structs.h

// 10.5 UNIX03 member name prefixes
#define DU3_PREFIX(s, m) __##s.__##m

#define context_x uc_mcontext->DU3_PREFIX(ss, x)
#define context_fp uc_mcontext->DU3_PREFIX(ss, fp)
#define context_lr uc_mcontext->DU3_PREFIX(ss, lr)
#define context_sp uc_mcontext->DU3_PREFIX(ss, sp)
#define context_pc uc_mcontext->DU3_PREFIX(ss, pc)
#define context_cpsr uc_mcontext->DU3_PREFIX(ss, cpsr)
#define context_esr uc_mcontext->DU3_PREFIX(es, esr)
#endif

#ifdef __FreeBSD__
#define context_x uc_mcontext.mc_gpregs.gp_x
#define context_fp context_x[REG_FP]
#define context_lr uc_mcontext.mc_gpregs.gp_lr
#define context_sp uc_mcontext.mc_gpregs.gp_sp
#define context_pc uc_mcontext.mc_gpregs.gp_elr
#endif

#ifdef __NetBSD__
#define context_x uc_mcontext.__gregs
#define context_fp uc_mcontext.__gregs[_REG_FP]
#define context_lr uc_mcontext.__gregs[_REG_LR]
#define context_sp uc_mcontext.__gregs[_REG_SP]
#define context_pc uc_mcontext.__gregs[_REG_ELR]
#endif

#ifdef __OpenBSD__
#define context_x sc_x
#define context_fp sc_x[REG_FP]
#define context_lr sc_lr
#define context_sp sc_sp
#define context_pc sc_elr
#endif

    pc = (void *)uc->context_pc;
#define storeReg(n, idx) registers[n] = (void *)uc->##idx;
    storeReg("lr", context_lr);
    for (int xi = 0; xi < 31; xi++)
    {
        storeReg(fmt::format("x{}", xi), context_x[xi]);
    }
#else
    pc = (void *)uc->uc_mcontext.pc;
    for (int xi = 0; xi < 31; xi++)
    {
        registers[fmt::format("x{}", xi)] = (void *)uc->uc_mcontext.regs[xi];
    }
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
    static const int signals_to_handle[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGTRAP, SIGABRT, 0};
    for (int i = 0; signals_to_handle[i] != 0; i++)
    {
        struct sigaction sigAct, oldSigAct;
        install_sigaction_signal_handler(&sigAct, &oldSigAct, signals_to_handle[i], crash_handler);
    }
}
} // namespace openminecraft::vm::pixeltower::v1::tracing
