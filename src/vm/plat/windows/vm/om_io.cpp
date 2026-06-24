#include "openminecraft/vm/os/om_io.hpp"
#include <iostream>
#include <windows.h>

namespace openminecraft::vm::os
{
	void write(uint64_t fd, uint8_t* src, int off, int len, bool append)
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

	int read(uint64_t fd, uint8_t* src, int off, int len)
	{
        HANDLE h = (HANDLE)fd;
        DWORD rd = 0;
        return ReadFile(h, src + off, len, &rd, nullptr) ? rd : -1;
	}
}