#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include <libloaderapi.h>
#include <windows.h>
#include <winnls.h>

namespace openminecraft::vm::elysia
{
typedef HRESULT(WINAPI *SetThreadDescriptionProc)(HANDLE, PCWSTR);
static SetThreadDescriptionProc func = nullptr;
static bool checked = false;

inline static void checkFunc()
{
    if (checked)
    {
        return;
    }

    HMODULE m = GetModuleHandleW(L"kernel32.dll");
    if (m)
    {
        func = (SetThreadDescriptionProc)GetProcAddress(m, "SetThreadDescription");
    }
    checked = true;
}

inline static std::wstring CharToWide(const char *source, UINT codePage = CP_UTF8)
{
    if (!source)
        return L"";

    int sizeNeeded = MultiByteToWideChar(codePage, 0, source, -1, nullptr, 0);
    if (sizeNeeded <= 0)
        return L"";

    std::wstring result(sizeNeeded - 1, L'\0');
    MultiByteToWideChar(codePage, 0, source, -1, &result[0], sizeNeeded);
    return result;
}

std::string OMElysiaThread::getName()
{
    return threadName;
}
void OMElysiaThread::setName(std::string n)
{
    threadName = n;
    checkFunc();
    if (func)
    {
        func((HANDLE)nativeHandle, CharToWide(n.c_str()).c_str());
    }
}
void OMElysiaThread::initInternals()
{
    nativeHandle = (void *)GetCurrentThread();
}
} // namespace openminecraft::vm::elysia
