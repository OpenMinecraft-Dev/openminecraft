#include "openminecraft/vm/pixeltower/v1/om_pixeltower_tracing.hpp"
#include "sys/signal.h"
#include <csignal>
#include <iostream>
#include <signal.h>

namespace openminecraft::vm::pixeltower::v1::tracing
{
static void crash_handler(int sig, siginfo_t *info, void *context)
{
    ucontext_t *const uc = (ucontext_t *)context;
#ifdef __x86_64__
#if defined(__APPLE__)
    std::cout << uc->uc_mcontext->__ss.__rip << std::endl;
#else
    std::cout << uc->uc_mcontext.gregs[REG_RIP] << std::endl;
#endif
#endif

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
