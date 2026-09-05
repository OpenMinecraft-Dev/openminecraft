#include "openminecraft-shell/application.hpp"
#include "openminecraft/geom/om_svg_structure.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <csignal>
#include <iostream>
#include <string>
#include "openminecraft/log/om_log_common.hpp"

extern "C"
{
    extern const char _binary_boot_bundle_start[];
    extern const char _binary_boot_bundle_end[];
    extern const char _binary_external_bundle_start[];
    extern const char _binary_external_bundle_end[];
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
    openminecraft::vfs::fsmountZipArchive(
        _binary_boot_bundle_start, (uint32_t)(_binary_boot_bundle_end - _binary_boot_bundle_start), "/bootassets");
    openminecraft::vfs::fsmountZipArchive(_binary_external_bundle_start,
                                          (uint32_t)(_binary_external_bundle_end - _binary_external_bundle_start),
                                          "/external");
    openminecraftshell::OMApplication app(a);
    /*int re = app.entry();
    logger.info("Kernel exited with code {}", re);

    return re;*/
    openminecraft::geom::parseSvgPath(
        "M3.25 4.8c.515 0 .773 0 .955.129q.097.069.166.166c.13.182.129.44.129.955v.7c0 .515 0 .773-.129.955a.7.7 0 0 "
        "1-.166.166c-.182.13-.44.129-.955.129s-.773 0-.955-.129a.7.7 0 0 1-.166-.166C1.999 7.523 2 7.265 2 "
        "6.75v-.7c0-.515 0-.773.129-.955a.7.7 0 0 1 .166-.166c.182-.13.44-.13.955-.13M7.1 6.2c.375 0 .563 0 "
        ".694.096a.5.5 0 0 1 .11.11C8 6.538 8 6.725 8 7.1s0 .563-.096.694a.5.5 0 0 1-.11.11C7.663 8 7.474 8 7.1 "
        "8h-.8c-.375 0-.562 0-.694-.096a.5.5 0 0 1-.11-.11C5.4 7.663 5.4 7.474 5.4 7.1c0-.375 0-.562.095-.694a.5.5 0 0 "
        "1 .111-.11c.132-.096.319-.096.694-.096zM6.75 2c.515 0 .773 0 "
        ".955.129q.097.069.166.166c.13.182.129.44.129.955v.7c0 .515 0 .773-.129.955a.7.7 0 0 "
        "1-.166.166c-.182.13-.44.13-.955.13s-.773 0-.955-.13a.7.7 0 0 "
        "1-.166-.166c-.13-.182-.129-.44-.129-.955v-.7c0-.515 0-.773.129-.955a.7.7 0 0 1 "
        ".166-.166c.182-.13.44-.129.955-.129M3.6 2c.375 0 .563 0 .694.096a.5.5 0 0 1 .11.11c.096.131.096.32.096.694 0 "
        ".375 0 .562-.096.694a.5.5 0 0 1-.11.11c-.131.096-.32.096-.694.096h-.7c-.375 0-.563 0-.694-.096a.5.5 0 0 "
        "1-.11-.11C2 3.462 2 3.275 2 2.9s0-.563.096-.694a.5.5 0 0 1 .11-.11C2.337 2 2.525 2 2.899 2z");
    return 0;
}
