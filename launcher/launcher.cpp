#include "boost/stacktrace/stacktrace.hpp"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <csignal>
#include <iostream>

auto logger = openminecraft::log::OMLogger("launcher");

void handle(int sig)
{
    std::cout << boost::stacktrace::stacktrace();
    logger.fatal("!! KERNEL CRASHED !!");
}

int main(int argc, char **argv)
{
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND | SA_STACK;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handle;

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);

    std::vector<std::string> a(argc);
    logger.info("Args:");
    for (int i = 0; i < argc; i++)
    {
        a.push_back(argv[i]);
        logger.info(argv[i]);
    }
    logger.info("Booting kernel...");
    int re = openminecraft::boot::boot(a);
    logger.info("Kernel exited with code {}", re);
    return re;
}