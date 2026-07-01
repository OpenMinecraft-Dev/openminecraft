#ifndef OM_MEM_RECORD_HPP
#define OM_MEM_RECORD_HPP

#include <cstdlib>
#include <functional>
#include <string>
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
    const char *tag;
};

size_t heapSize(void *p);
void rec(MemModifyInfo &&i);
void printres(std::function<void(std::string, std::string)>);
} // namespace openminecraft::mem::castorice

#endif
