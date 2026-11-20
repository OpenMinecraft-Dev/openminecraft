#include "openminecraft/mem/om_mem_saferead.hpp"
#include <csetjmp>
#include <csignal>
#include <cstdint>
#include <optional>
#if defined(OM_PLATFORM_WINDOWS) || defined(OM_PLATFORM_MINGW)
#include "windows.h"
#endif

namespace openminecraft::mem
{
#if defined(OM_PLATFORM_UNIX)
static sigjmp_buf env;

static void crashHandler(int)
{
    signal(SIGSEGV, SIG_IGN);
    siglongjmp(env, 1);
}
#endif

template <typename T> auto safeRead(void *p) -> std::optional<T>
{
#if defined(OM_PLATFORM_UNIX)
    signal(SIGSEGV, crashHandler);
    T v;
    if (!sigsetjmp(env, 1))
    {
        v = *reinterpret_cast<T *>(p);
    }
    else
    {
        signal(SIGSEGV, nullptr);
        return std::nullopt;
    }

    signal(SIGSEGV, nullptr);
    return v;
#else
    /*__try
    {
        return *reinterpret_cast<T *>(p);
    }
    __except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER
                                                                 : EXCEPTION_CONTINUE_SEARCH)
    {
        return std::nullopt;
    }*/
    T v;
    SIZE_T bytesRead = 0;
    if (ReadProcessMemory(GetCurrentProcess(), p, &v, sizeof(T), &bytesRead) && bytesRead == sizeof(T))
    {
        return v;
    }
    return std::nullopt;
#endif
}

template std::optional<void *> safeRead<void *>(void *);
template std::optional<uintptr_t> safeRead<uintptr_t>(void *);
template std::optional<int8_t> safeRead<int8_t>(void *);
template std::optional<int16_t> safeRead<int16_t>(void *);
template std::optional<int32_t> safeRead<int32_t>(void *);
template std::optional<int64_t> safeRead<int64_t>(void *);
} // namespace openminecraft::mem
