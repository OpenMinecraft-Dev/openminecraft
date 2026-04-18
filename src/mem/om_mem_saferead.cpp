#include "openminecraft/mem/om_mem_saferead.hpp"
#include <csetjmp>
#include <csignal>
#include <cstdint>
#include <optional>

namespace openminecraft::mem
{
static sigjmp_buf env;

static void crashHandler(int)
{
    siglongjmp(env, 1);
}

std::optional<uint8_t> safeRead(void *p)
{
    signal(SIGSEGV, crashHandler);
    uint8_t v;
    if (sigsetjmp(env, 1) == 0)
    {
        v = *reinterpret_cast<uint8_t *>(p);
    }
    else
    {
        signal(SIGSEGV, nullptr);
        return std::nullopt;
    }

    signal(SIGSEGV, nullptr);
    return v;
}
} // namespace openminecraft::mem
