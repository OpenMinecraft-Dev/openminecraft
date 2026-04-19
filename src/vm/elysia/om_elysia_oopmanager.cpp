#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <cstring>

namespace openminecraft::vm::elysia
{
OMElysiaOopManager::OMElysiaOopManager(OMElysiaVirtualWorld *vw) : world(vw)
{
}
OMElysiaOopManager::~OMElysiaOopManager()
{
}

uint64_t OMElysiaOopManager::oopHeaderLength()
{
    return sizeof(uint32_t) + world->metaspaceHeap.ptrLength();
}
OMElysiaOop *OMElysiaOopManager::allocateOop(OMElysiaKlass *klass)
{
    auto ll = reinterpret_cast<OMElysiaOop *>(
        world->mainHeap.allocate(oopHeaderLength() + reinterpret_cast<OMElysiaInstanceKlass *>(klass)->length));
    std::memset(ll, 0x00, oopHeaderLength() + reinterpret_cast<OMElysiaInstanceKlass *>(klass)->length);
    if (world->metaspaceHeap.enablePtrCompress())
    {
        ll->compressed = world->metaspaceHeap.compress(klass);
    }
    else
    {
        ll->raw = klass;
    }
    return ll;
}

uintptr_t OMElysiaOopManager::oopAccessField(void *base, uint64_t offset)
{
    return reinterpret_cast<uintptr_t>(base) + oopHeaderLength() + offset;
}
} // namespace openminecraft::vm::elysia
