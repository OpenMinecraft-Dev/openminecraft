#include "openminecraft/mem/om_mem_record.hpp"

#include "stdio.h"

namespace openminecraft::mem::allocator {
void rec(MemModifyInfo i)
{
    printf("%s %p %ld\n", i.type == Free ? "f" : "a", i.addr, i.length);
}
}