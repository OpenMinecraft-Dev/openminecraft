#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include <cstring>

using namespace openminecraft::binary::hash;

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
    return world->metaspaceHeap.enablePtrCompress() ? sizeof(OMElysiaOopCompressed) : sizeof(OMElysiaOopUncompressed);
}
uint64_t OMElysiaOopManager::oopArrayHeaderLength()
{
    return world->metaspaceHeap.enablePtrCompress() ? sizeof(OMElysiaArrayOopCompressed)
                                                    : sizeof(OMElysiaArrayOopUncompressed);
}
OMElysiaOop *OMElysiaOopManager::allocateOop(OMElysiaKlass *klass)
{
    auto ll = reinterpret_cast<OMElysiaOop *>(
        world->mainHeap.allocate(oopHeaderLength() + reinterpret_cast<OMElysiaInstanceKlass *>(klass)->length));
    ll->markword &= markEden;
    std::memset(ll, 0x00, oopHeaderLength() + reinterpret_cast<OMElysiaInstanceKlass *>(klass)->length);
    if (world->metaspaceHeap.enablePtrCompress())
    {
        reinterpret_cast<OMElysiaOopCompressed *>(ll)->klass = world->metaspaceHeap.compress(klass);
    }
    else
    {
        reinterpret_cast<OMElysiaOopUncompressed *>(ll)->klass = klass;
    }
    return ll;
}

OMElysiaKlass *OMElysiaOopManager::oopGetKlass(void *base)
{
    if (world->metaspaceHeap.enablePtrCompress())
    {
        return reinterpret_cast<OMElysiaKlass *>(
            world->metaspaceHeap.decompress(reinterpret_cast<OMElysiaOopCompressed *>(base)->klass));
    }
    else
    {
        return reinterpret_cast<OMElysiaOopUncompressed *>(base)->klass;
    }
}

uintptr_t OMElysiaOopManager::oopAccessField(void *base, uint64_t offset)
{
    return reinterpret_cast<uintptr_t>(base) + oopHeaderLength() + offset;
}

OMElysiaArrayOop *OMElysiaOopManager::allocateArr(OMElysiaArrayKlass *klass, jint length)
{
    int i = 0;
    switch (hash_compile_time(klass->lowerDim->name))
    {
    case "byte"_hash:
    case "boolean"_hash:
        i = 1;
        break;
    case "char"_hash:
    case "short"_hash:
        i = 2;
        break;
    case "int"_hash:
    case "float"_hash:
        i = 4;
        break;
    case "long"_hash:
    case "double"_hash:
        i = 8;
        break;
    default:
        i = klass->ptrLength;
        break;
    }

    auto ll = reinterpret_cast<OMElysiaArrayOop *>(world->mainHeap.allocate(oopArrayHeaderLength() + i * length));
    std::memset(ll, 0x00, oopArrayHeaderLength() + i * length);

    ll->length = length;
    if (world->metaspaceHeap.enablePtrCompress())
    {
        reinterpret_cast<OMElysiaArrayOopCompressed *>(ll)->klass = world->metaspaceHeap.compress(klass);
    }
    else
    {
        reinterpret_cast<OMElysiaArrayOopUncompressed *>(ll)->klass = klass;
    }

    return ll;
}
} // namespace openminecraft::vm::elysia
