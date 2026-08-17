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

auto read(uint64_t fd, uint8_t *src, int off, int len) -> int
{
    return ::read(static_cast<int>(fd), src + off, len);
}

auto available(uint64_t fd) -> int
{
    int n = 0;
    if (ioctl(fd, FIONREAD, &n) < 0)
    {
        return 0;
    }

    return n;
}

auto convertHandle(int hnd) -> uint64_t
{
    return hnd;
}

auto open(const char *path, bool append) -> uint64_t
{
    return ::open(path, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC));
}

void close(uint64_t fd)
{
    ::close(fd);
}
} // namespace openminecraft::vm::os
