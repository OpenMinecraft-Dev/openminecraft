#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include <csetjmp>

auto logger = openminecraft::log::OMLogger("launcher");

int main(int argc, char **argv)
{
    openminecraft::log::multithread::registerCurrentThreadName("launcher");

    std::vector<std::string> a;
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
