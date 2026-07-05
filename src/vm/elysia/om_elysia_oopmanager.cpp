#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/util/om_util_encoding_utf.hpp"
#include "openminecraft/vm/elysia/interface/om_elysia_interface_defs.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_klassloader.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"
#include <cstring>
#include <iostream>
#include <limits>

using namespace openminecraft::binary::hash;

namespace openminecraft::vm::elysia
{
OMElysiaOopManager::OMElysiaOopManager(OMElysium *elysium)
    : elysium(elysium), logger("OMElysiaOopManager", this), generator(), dis(0, std::numeric_limits<int>::max())
{
}
OMElysiaOopManager::~OMElysiaOopManager()
{
}

uint64_t OMElysiaOopManager::oopHeaderLength()
{
    return elysium->metaspaceHeap.enablePtrCompress() ? sizeof(OMElysiaOopCompressed) : sizeof(OMElysiaOopUncompressed);
}
uint64_t OMElysiaOopManager::oopArrayHeaderLength()
{
    return elysium->metaspaceHeap.enablePtrCompress() ? sizeof(OMElysiaArrayOopCompressed)
                                                      : sizeof(OMElysiaArrayOopUncompressed);
}
OMElysiaOop *OMElysiaOopManager::allocateOop(OMElysiaKlass *klass, uint64_t length)
{
    auto ll = reinterpret_cast<OMElysiaOop *>(elysium->mainHeap.allocate(
        oopHeaderLength() + reinterpret_cast<OMElysiaInstanceKlass *>(klass)->length + length));
    std::memset(ll, 0x00, oopHeaderLength() + reinterpret_cast<OMElysiaInstanceKlass *>(klass)->length + length);
    if (elysium->metaspaceHeap.enablePtrCompress())
    {
        reinterpret_cast<OMElysiaOopCompressed *>(ll)->klass = elysium->metaspaceHeap.compress(klass);
    }
    else
    {
        reinterpret_cast<OMElysiaOopUncompressed *>(ll)->klass = klass;
    }

    ll->markword = dis(generator);
    return ll;
}

OMElysiaOop *OMElysiaOopManager::allocateString(std::string target)
{
    auto hsh = hash_compile_time(target.c_str());
    if (elysium->stringPool.count(hsh))
    {
        return elysium->stringPool[hsh];
    }
    auto stringKlass = elysium->klassLoader->findClass("java/lang/String")->toInstance();
    auto charArrKlass = elysium->klassLoader->findClass("[C")->toArray();

    auto u16target = util::encoding::utf8ToUtf16New(target);

    auto arr = allocateArr(charArrKlass, std::get<jsize>(u16target));
    std::memcpy(arrAccess<jchar>(arr), std::get<jchar *>(u16target), std::get<jsize>(u16target) * sizeof(jchar));
    free(std::get<jchar *>(u16target));

    auto strWrp = allocateOop(stringKlass);
    oopAccessPointerField(strWrp, 0, arr);

    elysium->stringPool[hsh] = strWrp;

    return strWrp;
}

OMElysiaKlass *OMElysiaOopManager::oopGetKlass(OMElysiaOop *base)
{
    if (elysium->metaspaceHeap.enablePtrCompress())
    {
        return reinterpret_cast<OMElysiaKlass *>(
            elysium->metaspaceHeap.decompress(reinterpret_cast<OMElysiaOopCompressed *>(base)->klass));
    }
    else
    {
        return reinterpret_cast<OMElysiaOopUncompressed *>(base)->klass;
    }
}

void OMElysiaOopManager::oopAccessPointerField(OMElysiaOop *base, uint64_t offset, void *ptrToWrite)
{
    if (elysium->mainHeap.enablePtrCompress())
    {
        *reinterpret_cast<uint32_t *>(oopAccessField(base, offset)) = elysium->mainHeap.compress(ptrToWrite);
    }
    else
    {
        *reinterpret_cast<void **>(oopAccessField(base, offset)) = ptrToWrite;
    }
}
OMElysiaOop *OMElysiaOopManager::oopAccessPointerField(OMElysiaOop *base, uint64_t offset)
{
    if (elysium->mainHeap.enablePtrCompress())
    {
        return reinterpret_cast<OMElysiaOop *>(
            elysium->mainHeap.decompress(*reinterpret_cast<uint32_t *>(oopAccessField(base, offset))));
    }
    else
    {
        return *reinterpret_cast<OMElysiaOop **>(oopAccessField(base, offset));
    }
}

OMElysiaOop *OMElysiaOopManager::oopAccessPointerStaticField(OMElysiaKlass *kls, uint64_t offset)
{
    if (elysium->mainHeap.enablePtrCompress())
    {
        return reinterpret_cast<OMElysiaOop *>(elysium->mainHeap.decompress(
            *reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(kls->toInstance()->staticBlock) + offset)));
    }
    else
    {
        return *reinterpret_cast<OMElysiaOop **>(reinterpret_cast<uintptr_t>(kls->toInstance()->staticBlock) + offset);
    }
}
void OMElysiaOopManager::oopAccessPointerStaticField(OMElysiaKlass *kls, uint64_t offset, void *ptrToWrite)
{
    if (elysium->mainHeap.enablePtrCompress())
    {
        *reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(kls->toInstance()->staticBlock) + offset) =
            elysium->mainHeap.compress(ptrToWrite);
    }
    else
    {
        *reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(kls->toInstance()->staticBlock) + offset) = ptrToWrite;
    }
}

uintptr_t OMElysiaOopManager::oopAccessField(OMElysiaOop *base, uint64_t offset)
{
    return reinterpret_cast<uintptr_t>(base) + oopHeaderLength() + offset;
}

jint OMElysiaOopManager::arrLength(OMElysiaOop *base)
{
    if (elysium->metaspaceHeap.enablePtrCompress())
    {
        return reinterpret_cast<OMElysiaArrayOopCompressed *>(base)->length;
    }
    else
    {
        return reinterpret_cast<OMElysiaArrayOopUncompressed *>(base)->length;
    }
}

OMElysiaOop *OMElysiaOopManager::arrAccessPtr(OMElysiaOop *oop, jint index)
{
    if (elysium->metaspaceHeap.enablePtrCompress())
    {
        return reinterpret_cast<OMElysiaOop *>(elysium->mainHeap.decompress(arrAccess<uint32_t>(oop)[index]));
    }
    else
    {
        return arrAccess<OMElysiaOop *>(oop)[index];
    }
}
void OMElysiaOopManager::arrAccessPtr(OMElysiaOop *oop, jint index, OMElysiaOop *data)
{
    if (elysium->metaspaceHeap.enablePtrCompress())
    {
        arrAccess<uint32_t>(oop)[index] = elysium->mainHeap.compress(data);
    }
    else
    {
        arrAccess<OMElysiaOop *>(oop)[index] = data;
    }
}

static jint arrayKlassItemLength(OMElysiaArrayKlass *klass)
{
    jint i = 0;
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
    return i;
}

OMElysiaOop *OMElysiaOopManager::allocateArr(OMElysiaArrayKlass *klass, jint length)
{
    auto ll = reinterpret_cast<OMElysiaOop *>(
        elysium->mainHeap.allocate(oopArrayHeaderLength() + klass->itemLength * length));
    std::memset(ll, 0x00, oopArrayHeaderLength() + klass->itemLength * length);

    if (elysium->metaspaceHeap.enablePtrCompress())
    {
        reinterpret_cast<OMElysiaArrayOopCompressed *>(ll)->klass = elysium->metaspaceHeap.compress(klass);
        reinterpret_cast<OMElysiaArrayOopCompressed *>(ll)->length = length;
    }
    else
    {
        reinterpret_cast<OMElysiaArrayOopUncompressed *>(ll)->klass = klass;
        reinterpret_cast<OMElysiaArrayOopUncompressed *>(ll)->length = length;
    }

    ll->markword = dis(generator);

    return ll;
}

OMElysiaOop *OMElysiaOopManager::allocateMultiArr(OMElysiaArrayKlass *klass, jint dim, jint *lengths)
{
    if (dim <= 1)
    {
        return allocateArr(klass, *lengths);
    }
    auto oop = allocateArr(klass, *lengths);
    for (int i = 0; i < *lengths; ++i)
    {
        auto sub = allocateMultiArr(klass->lowerDim->toArray(), dim - 1, lengths + 1);
        arrAccessPtr(oop, i, sub);
    }
    return oop;
}

uint64_t OMElysiaOopManager::oopLength(OMElysiaOop *oop)
{
    auto klass = oopGetKlass(oop);
    if (klass->isInstance())
    {
        return elysium->mainHeap.align(oopHeaderLength() + klass->toInstance()->length);
    }
    else
    {
        return elysium->mainHeap.align(oopArrayHeaderLength() +
                                       arrLength(oop) * arrayKlassItemLength(klass->toArray()));
    }
}
} // namespace openminecraft::vm::elysia
