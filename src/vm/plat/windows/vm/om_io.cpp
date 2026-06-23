#include "openminecraft/vm/os/om_io.hpp"
#include <iostream>
#include <windows.h>

namespace openminecraft::vm::os
{
	void write(uint64_t fd, uint8_t* src, int off, int len, bool append)
	{
        HANDLE h;
		switch (fd) {
        case 0:
			h = GetStdHandle(STD_INPUT_HANDLE);
			break;
        case 1:
			h = GetStdHandle(STD_OUTPUT_HANDLE);
            break;
        case 2:
			h = GetStdHandle(STD_ERROR_HANDLE);
			break;
        default:
            h = (HANDLE)fd;
            break;
		}

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
}
