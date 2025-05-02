#include "boost/stacktrace/stacktrace.hpp"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <csignal>
#include <cstdlib>
#include <iostream>

auto logger = openminecraft::log::OMLogger("launcher");

void handle(int sig)
{
    std::cout << boost::stacktrace::stacktrace();
    logger.fatal("!! KERNEL CRASHED !!");
    exit(-1);
}

int main(int argc, char **argv)
{
    signal(SIGSEGV, handle);
    signal(SIGABRT, handle);

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