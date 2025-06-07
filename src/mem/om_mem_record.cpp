#include "openminecraft/mem/om_mem_record.hpp"

#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"

uint64_t mems[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
std::string memName[10] = {
    "C++ allocator", 
    "SDL", 
    "Vulkan", 
    "Vulkan Internal", 
    "Unknown", 
    "Unknown", 
    "Unknown", 
    "Unknown", 
    "Unknown", 
    "Unknown"
};
uint64_t blocks = 0;

using namespace openminecraft::i18n::res;
namespace openminecraft::mem::castorice
{
log::OMLogger logger("Memory Record/Castorice");
void rec(MemModifyInfo i)
{
    i.type == Free ? (mems[i.tag] -= i.length) : (mems[i.tag] += i.length);
    i.type == Free ? (blocks--) : (blocks++);
}

// TODO: Precision lost !!!
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
    else if (l < 1024l * 1024 * 1024 * 1024)
    {
        return fmt::format("{:.{}f} GB", (double)l / 1024 / 1024 / 1024, 2);
    }
    else {
        return fmt::format("{:.{}f} TB", (double)l / 1024 / 1024 / 1024 / 1024, 2);
    }
}

void printres()
{
    logger.info(translate("openminecraft.mem.title"));
    logger.info(translate("openminecraft.mem.blocks", blocks));
    uint64_t data = 0;
    for (int i = 0; i < 10; i++)
    {
        logger.info(translate("openminecraft.mem.detail", memName[i], toDataSize(mems[i])));
        data += mems[i];
    }
    logger.info(translate("openminecraft.mem.detail", "*", toDataSize(data)));
    logger.info(translate("openminecraft.mem.detail", "/*", toDataSize(data / blocks)));
}
} // namespace openminecraft::mem::castorice