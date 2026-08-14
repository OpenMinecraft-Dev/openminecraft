#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <csignal>

extern "C"
{
    extern uint8_t _binary_boot_bundle_start[];
    extern uint8_t _binary_boot_bundle_end[];
}

auto logger = openminecraft::log::OMLogger("launcher");

void sighnd(int i)
{
    logger.fatal("signal handler {}", i);
    logger.dumpStacktrace();
    exit(-1);
}

auto main(int argc, char **argv) -> int
{
    openminecraft::log::multithread::registerCurrentThreadName("launcher");

    signal(SIGSEGV, sighnd);

    std::vector<std::string> a;
    logger.info("Args:");
    for (int i = 0; i < argc; i++)
    {
        a.emplace_back(argv[i]);
        logger.debug(argv[i]);
    }
    logger.info("Booting kernel...");
    openminecraft::vfs::fsmountBundle(
        std::make_shared<openminecraft::specs::vfsbundle::OMBundle>(
            _binary_boot_bundle_start, (uint32_t)(_binary_boot_bundle_end - _binary_boot_bundle_start)),
        "/bootassets");
    int re = openminecraft::boot::boot(a);
    logger.info("Kernel exited with code {}", re);

    return re;
}
