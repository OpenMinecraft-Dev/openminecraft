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
static void crash_handler(int sig, siginfo_t *info, void *context)
{
    ucontext_t *const uc = (ucontext_t *)context;
    log::OMLogger l("Crash Handler");
    l.info("errno: {}, signo: {}, code: {}, {}", info->si_errno, info->si_signo, info->si_code, getpid());

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
