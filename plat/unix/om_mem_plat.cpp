#include "openminecraft/mem/om_mem_record.hpp"
#if defined(OM_PLATFORM_IOS) || defined(OM_PLATFORM_MACOS)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif

namespace openminecraft::mem::allocator {
size_t heapSize(void* p)
{
    return malloc_usable_size(p);
}
}