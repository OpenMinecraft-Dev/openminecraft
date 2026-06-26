#ifndef OM_IO_HPP
#define OM_IO_HPP

#include <cstdint>
namespace openminecraft::vm::os
{
void write(uint64_t fd, uint8_t *src, int off, int len, bool append);
int read(uint64_t fd, uint8_t *src, int off, int len);
int available(uint64_t fd);
uint64_t convertHandle(int hnd);
} // namespace openminecraft::vm::os

#endif
