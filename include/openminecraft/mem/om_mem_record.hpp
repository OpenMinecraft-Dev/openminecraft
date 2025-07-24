#ifndef OM_MEM_RECORD_HPP
#define OM_MEM_RECORD_HPP

#include <cstdint>
#include <cstdlib>
#define OM_MEM_CPP 0
#define OM_MEM_SDL 1
#define OM_MEM_VULKAN 2
#define OM_MEM_VULKAN_INTERNAL 3
#define OM_MEM_OPENGL 4
#define OM_MEM_HARFBUZZ 5
#define OM_MEM_FREETYPE 6
#define OM_MEM_VMDATA 7
#define OM_MEM_VMCODE 8
namespace openminecraft::mem::castorice
{
enum MemModifyType
{
    Allocation,
    Free
};

struct MemModifyInfo
{
    MemModifyType type;
    void *addr;
    size_t length;
    uint8_t tag;
};

size_t heapSize(void *p);
void rec(MemModifyInfo i);
void printres();
uint64_t fetchSize(int t);
} // namespace openminecraft::mem::castorice

#endif