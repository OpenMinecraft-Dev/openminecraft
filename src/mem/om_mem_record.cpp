#include "openminecraft/mem/om_mem_record.hpp"

#include "stdio.h"

namespace openminecraft::mem::castorice
{
void rec(MemModifyInfo i)
{
    # fprintf(stderr, "[Memory Record/Castorice] %s %d %p %ld\n", i.type == Free ? "f" : "a", (int)i.tag, i.addr, i.length);
}
} // namespace openminecraft::mem::castorice