#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include <csignal>
#include <cstdlib>

auto logger = openminecraft::log::OMLogger("launcher");

void handle(int sig)
{
    logger.fatal("!! KERNEL CRASHED !!");
    logger.dumpStacktrace();
    exit(-1);
}

int main(int argc, char **argv)
{
    openminecraft::log::multithraad::registerCurrentThreadName("launcher");
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
    raise(SIGSEGV);
    return re;
}