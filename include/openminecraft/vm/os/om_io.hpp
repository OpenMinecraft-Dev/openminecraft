#ifndef OM_IO_HPP
#define OM_IO_HPP

#include <cstdint>
namespace openminecraft::vm::os
{
void write(uint64_t fd, uint8_t *src, int off, int len, bool append);
}

#endif
