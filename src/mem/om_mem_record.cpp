#include "openminecraft/mem/om_mem_record.hpp"

#include "openminecraft/log/om_log_common.hpp"
#include <cstddef>
#include <cstdio>
#include <string>

#if defined(OM_PLATFORM_IOS) || defined(OM_PLATFORM_MACOS)
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

namespace openminecraft::mem::castorice
{
struct OMMemEntry
{
    const char *tag;
    uint64_t size;
};
int entryLength = 0;
OMMemEntry *entries = nullptr;
uint64_t blocks = 0;

log::OMLogger logger("Memory Record/Castorice");
void rec(MemModifyInfo i)
{
begin:
    for (int id = 0; id < entryLength; id++)
    {
        if (std::strcmp(entries[id].tag, i.tag) == 0)
        {
            if (i.type == Free)
            {
                if (entries[id].size <= i.length)
                {
                    entries[id].size = 0;
                }
                else
                {
                    entries[id].size -= i.length;
                }
                blocks--;
            }
            else
            {
                entries[id].size += i.length;
                blocks++;
            }

            return;
        }
    }

    entryLength++;
    entries = entries ? reinterpret_cast<OMMemEntry *>(realloc(entries, sizeof(OMMemEntry) * entryLength))
                      : reinterpret_cast<OMMemEntry *>(calloc(entryLength, sizeof(OMMemEntry)));
    entries[entryLength - 1].tag = i.tag;
    entries[entryLength - 1].size = 0;
    goto begin;
}

std::string toDataSize(uint64_t l)
{
    if (l < 1024)
    {
        return fmt::format("{} B", l);
    }
    else if (l < 1024 * 1024)
    {
        return fmt::format("{:.{}f} KB", (double)l / 1024, 2);
    }
    else if (l < 1024 * 1024 * 1024)
    {
        return fmt::format("{:.{}f} MB", (double)l / 1024 / 1024, 2);
    }
    else if (l < 1024ll * 1024 * 1024 * 1024)
    {
        return fmt::format("{:.{}f} GB", (double)l / 1024 / 1024 / 1024, 2);
    }
    else
    {
        return fmt::format("{:.{}f} TB", (double)l / 1024 / 1024 / 1024 / 1024, 2);
    }
}

size_t heapSize(void *p)
{
    if (!p)
    {
        return 0;
    }
#if defined(OM_PLATFORM_IOS) || defined(OM_PLATFORM_MACOS)
    return malloc_size(p);
#elif defined(OM_PLATFORM_WINDOWS)
    return _msize(p);
#else
    return malloc_usable_size(p);
#endif
}

void printres()
{
    logger.info("Memory blocks");
    logger.info("{} blocks recorded", blocks);
    uint64_t data = 0;
    for (int i = 0; i < entryLength; i++)
    {
        data += entries[i].size;
        logger.info("{} => {}", entries[i].tag, toDataSize(entries[i].size));
    }
    logger.info("* => {}", toDataSize(data));
}
} // namespace openminecraft::mem::castorice
