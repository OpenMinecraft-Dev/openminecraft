#include "openminecraft/mem/om_mem_record.hpp"

#include "stdio.h"

namespace openminecraft::mem::allocator
{
void rec(MemModifyInfo i)
{
    // printf("%s %d %p %ld\n", i.type == Free ? "f" : "a", (int)i.tag, i.addr, i.length);
}
} // namespace openminecraft::mem::allocator