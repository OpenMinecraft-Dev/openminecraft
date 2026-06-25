#include "openminecraft/vm/os/om_io.hpp"
#include <sys/ioctl.h>
#include <unistd.h>

namespace openminecraft::vm::os
{
void write(uint64_t fd, uint8_t *src, int off, int len, bool append)
{
    ::write(static_cast<int>(fd), src + off, len);
}

int read(uint64_t fd, uint8_t *src, int off, int len)
{
    return ::read(static_cast<int>(fd), src + off, len);
}

int available(uint64_t fd)
{
    int n = 0;
    if (ioctl(fd, FIONREAD, &n) < 0)
    {
        return 0;
    }

    return n;
}
} // namespace openminecraft::vm::os
