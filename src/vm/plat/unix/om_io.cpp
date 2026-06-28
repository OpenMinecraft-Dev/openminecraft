#include "openminecraft/vm/os/om_io.hpp"
#include <fcntl.h>
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

uint64_t convertHandle(int hnd)
{
    return hnd;
}

uint64_t open(const char *path, bool append)
{
    return ::open(path, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC));
}

void close(uint64_t fd)
{
    ::close(fd);
}
} // namespace openminecraft::vm::os
