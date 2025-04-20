#include "openminecraft/mem/om_mem_record.hpp"
#if defined(OM_PLATFORM_IOS) || defined(OM_PLATFORM_MACOS)
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

namespace openminecraft::mem::allocator {
size_t heapSize(void* p)
{
#if defined(OM_PLATFORM_IOS) || defined(OM_PLATFORM_MACOS)
    return malloc_size(p);
#else
    return malloc_usable_size(p);
#endif
}
}