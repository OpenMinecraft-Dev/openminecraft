#include "openminecraft/mem/om_mem_record.hpp"
#include <malloc.h>

namespace openminecraft::mem::castorice
{
size_t heapSize(void *p)
{
    return _msize(p);
}
} // namespace openminecraft::mem::castorice