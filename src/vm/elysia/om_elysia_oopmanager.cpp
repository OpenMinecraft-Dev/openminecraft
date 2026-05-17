#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"
#include "openminecraft/vm/encoding/om_encoding_utf.hpp"
#include <cstring>

using namespace openminecraft::binary::hash;

namespace openminecraft::vm::elysia
{
OMElysiaOopManager::OMElysiaOopManager(OMElysiaVirtualWorld *vw) : world(vw), logger("OMElysiaOopManager", this)
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

OMElysiaOop *OMElysiaOopManager::allocateString(std::string &target)
{
    auto nativeKlassloader = world->klassLoader;
    auto stringKlass = nativeKlassloader->findClass("java/lang/String")->toInstance();
    auto charArrKlass = nativeKlassloader->findClass("[C")->toArray();

    auto u16target = encoding::utf32ToUtf16(encoding::utf8ToUtf32(target));

    auto oopM = nativeKlassloader->upper()->oopManager;

    auto arr = oopM->allocateArr(charArrKlass, u16target.size());
    for (int i = 0; i < u16target.size() / 2; i++)
    {
        oopM->arrAccess<jchar>(arr)[i] = static_cast<jchar>(u16target[i * 2]) << 8 | u16target[i * 2 + 1];
    }

    auto strWrp = oopM->allocateOop(stringKlass);
    oopM->oopAccessPointerField(strWrp, 0, arr);

    return strWrp;
}

OMElysiaKlass *OMElysiaOopManager::oopGetKlass(OMElysiaOop *base)
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

void OMElysiaOopManager::oopAccessPointerField(OMElysiaOop *base, uint64_t offset, void *ptrToWrite)
{
    if (world->mainHeap.enablePtrCompress())
    {
        *reinterpret_cast<uint32_t *>(oopAccessField(base, offset)) = world->mainHeap.compress(ptrToWrite);
    }
    else
    {
        *reinterpret_cast<void **>(oopAccessField(base, offset)) = ptrToWrite;
    }
}
OMElysiaOop *OMElysiaOopManager::oopAccessPointerField(OMElysiaOop *base, uint64_t offset)
{
    if (world->mainHeap.enablePtrCompress())
    {
        return reinterpret_cast<OMElysiaOop *>(
            world->mainHeap.decompress(*reinterpret_cast<uint32_t *>(oopAccessField(base, offset))));
    }
    else
    {
        return *reinterpret_cast<OMElysiaOop **>(oopAccessField(base, offset));
    }
}

uintptr_t OMElysiaOopManager::oopAccessField(OMElysiaOop *base, uint64_t offset)
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
