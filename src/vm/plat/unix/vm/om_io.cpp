#include "openminecraft/vm/os/om_io.hpp"
#include <unistd.h>

namespace openminecraft::vm::os
{
void write(uint64_t fd, uint8_t *src, int off, int len, bool append)
{
    ::write(static_cast<int>(fd), src + off, len);
}
} // namespace openminecraft::vm::os
