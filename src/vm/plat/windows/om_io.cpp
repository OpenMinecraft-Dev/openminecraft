#include "openminecraft/vm/os/om_io.hpp"
#include <cstdint>
#include <io.h>
#include <iostream>
#include <windows.h>

namespace openminecraft::vm::os
{
void write(uint64_t fd, uint8_t *src, int off, int len, bool append)
{
    HANDLE h = (HANDLE)fd;

    auto ptr = src + off;
    while (len > 0)
    {
        DWORD written;
        if (!WriteFile(h, ptr, len, &written, nullptr))
        {
            std::cout << "WriteFile failed with error: " << GetLastError() << std::endl;
            break;
        }
        len -= written;
        ptr += written;
    }
}

int read(uint64_t fd, uint8_t *src, int off, int len)
{
    HANDLE h = (HANDLE)fd;
    DWORD rd = 0;
    return ReadFile(h, src + off, len, &rd, nullptr) ? rd : -1;
}

int available(uint64_t fd)
{
    HANDLE h = (HANDLE)fd;
    DWORD type = GetFileType(h);

    if (type == FILE_TYPE_DISK)
    {
        LARGE_INTEGER fileSize, position;
        if (GetFileSizeEx(h, &fileSize))
        {
            LARGE_INTEGER distanceToMove = {0};
            if (SetFilePointerEx(h, distanceToMove, &position, FILE_CURRENT))
            {
                return (int)(fileSize.QuadPart - position.QuadPart);
            }
        }
        return 0;
    }
    else if (type == FILE_TYPE_PIPE)
    {
        DWORD nAvail = 0;
        if (PeekNamedPipe(h, NULL, 0, NULL, &nAvail, NULL))
        {
            return (int)nAvail;
        }
        return 0;
    }
    else
    {
        return 0;
    }
    return 0;
}

uint64_t convertHandle(int hnd)
{
    return (uint64_t)_get_osfhandle(hnd);
}
} // namespace openminecraft::vm::os
