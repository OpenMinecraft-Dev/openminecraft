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
static sigjmp_buf env;

static void crashHandler(int)
{
    siglongjmp(env, 1);
}
#endif

std::optional<uint8_t> safeRead(void *p)
{
#ifdef OM_PLATFORM_UNIX
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
#else
    __try
    {
        return *reinterpret_cast<uint8_t *>(p);
    }
    __except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return std::nullopt;
    }
#endif
}
} // namespace openminecraft::mem
