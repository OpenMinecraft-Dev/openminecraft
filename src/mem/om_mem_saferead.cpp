#include "openminecraft/mem/om_mem_saferead.hpp"
#include <csetjmp>
#include <csignal>
#include <cstdint>
#include <optional>
#ifdef OM_PLATFORM_WINDOWS
#include "windows.h"
#endif

namespace openminecraft::mem
{
#ifdef OM_PLATFORM_UNIX
static jmp_buf env;

static void crashHandler(int)
{
#ifdef OM_PLATFORM_BSD
    siglongjmp(&env, 1);
#else
    siglongjmp(env, 1);
#endif
}
#endif

std::optional<uint8_t> safeRead(void *p)
{
#ifdef OM_PLATFORM_UNIX
    signal(SIGSEGV, crashHandler);
    uint8_t v;
#ifdef OM_PLATFORM_BSD
    if (sigsetjmp(&env, 1) == 0)
#else
    if (sigsetjmp(env, 1) == 0)
#endif
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
#else
    __try
    {
        return *reinterpret_cast<uint8_t *>(p);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return std::nullopt;
    }
#endif
}
} // namespace openminecraft::mem
