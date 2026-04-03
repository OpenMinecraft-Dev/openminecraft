#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <iostream>

namespace openminecraft::vm::elysia
{
OMElysiaVirtualWorld::OMElysiaVirtualWorld()
    : metaspaceHeap("elysia_metaspace", 1024 * 1024 * 16), mainHeap("elysia_main", 1024 * 1024 * 1024)
{
    while (true)
    {
        auto ptr = metaspaceHeap.allocate(1024);
        auto ptr2 = metaspaceHeap.allocate(1024);
        if (!ptr)
        {
            throw 0;
        }
        std::cout << ptr << std::endl;
        metaspaceHeap.deallocate(ptr, 1024);
    }
}
OMElysiaVirtualWorld::~OMElysiaVirtualWorld()
{
}
} // namespace openminecraft::vm::elysia
