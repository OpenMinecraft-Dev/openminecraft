#include "openminecraft/mem/om_mem_record.hpp"
#include <malloc.h>

namespace openminecraft::mem::allocator {
size_t heapSize(void* p)
{
    return _msize(p);
}
}